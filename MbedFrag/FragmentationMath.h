/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2017 Semtech

Description: 	Firmware update over the air with LoRa proof of concept
				Functions for the decoding
*/

#ifndef _MBEDFRAG_FRAGMENTATION_MATH_H
#define _MBEDFRAG_FRAGMENTATION_MATH_H

#include "mbed.h"

#define FRAG_SESSION_ONGOING    0xffffffff

typedef struct
{
    int NbOfFrag;   // NbOfUtilFrames=SIZEOFFRAMETRANSMIT;
    int Redundancy; // nbr of extra frame
    int DataSize;   // included the lorawan specific data hdr,devadrr,...but without mic and payload decrypted
} FragmentationSessionParams_t;

// This file contains functions for the correction mechanisms designed by Semtech
class FragmentationMath
{
  public:
    /**
     * FragmentationMath
     * Initializes the magic Semtech library.
     *
     * @param flash          Instance of IFlash
     * @param frame_count    Number of expected fragments (without redundancy packets)
     * @param frame_size     Size of a fragment (without LoRaWAN header)
     * @param redundancy_max Maximum number of redundancy packets
     */
    FragmentationMath(IFlash *flash, uint16_t frame_count, uint8_t frame_size, uint16_t redundancy_max)
        : _flash(flash), _frame_count(frame_count), _frame_size(frame_size), _redundancy_max(redundancy_max)
    {
    }

    ~FragmentationMath()
    {
        if (matrixM2B)
        {
            free(matrixM2B);
        }
        if (missingFrameIndex)
        {
            free(missingFrameIndex);
        }

        if (matrixRow)
        {
            free(matrixRow);
        }
        if (matrixDataTemp)
        {
            free(matrixDataTemp);
        }
        if (dataTempVector)
        {
            free(dataTempVector);
        }
        if (dataTempVector2)
        {
            free(dataTempVector2);
        }
        if (s)
        {
            free(s);
        }
    }

    /**
     * Initialize the FragmentationMath library. This function allocates the required buffers.
     *
     * @returns true if the memory was allocated, false if one or more allocations failed
     */
    bool initialize()
    {
        // global for this session
        matrixM2B = (uint8_t *)calloc((_redundancy_max / 8) * _redundancy_max, 1);
        missingFrameIndex = (uint16_t *)calloc(_frame_count, sizeof(uint16_t));
        for (size_t ix = 0; ix < _frame_count; ix++)
        {
            missingFrameIndex[ix] = 1;
        }

        // these get reset for every frame
        matrixRow = (bool *)calloc(_frame_count, 1);
        matrixDataTemp = (uint8_t *)calloc(_frame_size, 1);
        dataTempVector = (bool *)calloc(_redundancy_max, 1);
        dataTempVector2 = (bool *)calloc(_redundancy_max, 1);
        s = (bool *)calloc(_redundancy_max, 1);

        numberOfLoosingFrame = 0;
        lastReceiveFrameCnt = 0;

        if (!matrixM2B || !missingFrameIndex || !matrixRow || !matrixDataTemp || !dataTempVector || !dataTempVector2 || !s)
        {
            printf("[FragmentationMath] Could not allocate memory...\n");
            return false;
        }

        return true;
    }

    /**
     * Let the library know that a frame was found.
     * @param frameCounter
     */
    void set_frame_found(uint16_t frameCounter)
    {
        missingFrameIndex[frameCounter - 1] = 0;

        FindMissingReceiveFrame(frameCounter);
    }

    /**
     * Process a redundancy frame
     *
     * @param frameCounter      The frameCounter for this frame
     * @param rowData           Binary data of the frame (without LoRaWAN header)
     * @param sFotaParameter    Current state of the fragmentation session
     *
     * @returns FRAG_SESSION_ONGOING if the packets are not completed yet,
                any other value between 0..FRAG_SESSION_ONGOING if the packet was deconstructed
     */
    int process_redundant_frame(uint16_t frameCounter, uint8_t *rowData, FragmentationSessionParams_t sFotaParameter)
    {
        int l;
        int i;
        int j;
        int li;
        int lj;
        int firstOneInRow;
        static int m2l = 0;
        int first = 0;
        int noInfo = 0;

        memset(matrixRow, 0, _frame_count);
        memset(matrixDataTemp, 0, _frame_size);
        memset(dataTempVector, 0, _redundancy_max);
        memset(dataTempVector2, 0, _redundancy_max);

        FindMissingReceiveFrame(frameCounter);

        FragmentationGetParityMatrixRow(frameCounter - sFotaParameter.NbOfFrag, sFotaParameter.NbOfFrag, matrixRow); //frameCounter-sFotaParameter.NbOfFrag

        for (l = 0; l < (sFotaParameter.NbOfFrag); l++)
        {
            if (matrixRow[l] == 1)
            {
                if (missingFrameIndex[l] == 0)
                { // xor with already receive frame
                    matrixRow[l] = 0;
                    GetRowInFlash(l, matrixDataTemp);
                    XorLineData(rowData, matrixDataTemp, sFotaParameter.DataSize);
                }
                else
                { // fill the "little" boolean matrix m2
                    dataTempVector[missingFrameIndex[l] - 1] = 1;
                    if (first == 0)
                    {
                        first = 1;
                    }
                }
            }
        }
        firstOneInRow = FindFirstOne(dataTempVector, numberOfLoosingFrame);
        if (first > 0)
        { //manage a new line in MatrixM2
            while (s[firstOneInRow] == 1)
            { // row already diagonalized exist&(sFotaParameter.MatrixM2[firstOneInRow][0])
                ExtractLineFromBinaryMatrix(dataTempVector2, firstOneInRow, numberOfLoosingFrame);
                XorLineBool(dataTempVector, dataTempVector2, numberOfLoosingFrame);
                li = FindMissingFrameIndex(firstOneInRow); // have to store it in the mi th position of the missing frame
                GetRowInFlash(li, matrixDataTemp);
                XorLineData(rowData, matrixDataTemp, sFotaParameter.DataSize);
                if (VectorIsNull(dataTempVector, numberOfLoosingFrame))
                {
                    noInfo = 1;
                    break;
                }
                firstOneInRow = FindFirstOne(dataTempVector, numberOfLoosingFrame);
            }
            if (noInfo == 0)
            {
                PushLineToBinaryMatrix(dataTempVector, firstOneInRow, numberOfLoosingFrame);
                li = FindMissingFrameIndex(firstOneInRow);
                StoreRowInFlash(rowData, li);
                s[firstOneInRow] = 1;
                m2l++;
            }

            if (m2l == numberOfLoosingFrame)
            { // then last step diagonalized
                if (numberOfLoosingFrame > 1)
                {
                    for (i = (numberOfLoosingFrame - 2); i >= 0; i--)
                    {
                        li = FindMissingFrameIndex(i);
                        GetRowInFlash(li, matrixDataTemp);
                        for (j = (numberOfLoosingFrame - 1); j > i; j--)
                        {
                            ExtractLineFromBinaryMatrix(dataTempVector2, i, numberOfLoosingFrame);
                            ExtractLineFromBinaryMatrix(dataTempVector, j, numberOfLoosingFrame);
                            if (dataTempVector2[j] == 1)
                            {
                                XorLineBool(dataTempVector2, dataTempVector, numberOfLoosingFrame);
                                PushLineToBinaryMatrix(dataTempVector2, i, numberOfLoosingFrame);

                                lj = FindMissingFrameIndex(j);

                                GetRowInFlash(lj, rowData);
                                XorLineData(matrixDataTemp, rowData, sFotaParameter.DataSize);
                            }
                        }
                        StoreRowInFlash(matrixDataTemp, li);
                    }
                    return (numberOfLoosingFrame);
                }

                else
                { //ifnot ( numberOfLoosingFrame > 1 )
                    return (numberOfLoosingFrame);
                }
            }
        }

        return 0xffffffff;
    }

    /**
     * Get the number of lost frames
     */
    int get_lost_frame_count()
    {
        return numberOfLoosingFrame;
    }

  private:
    void GetRowInFlash(int l, uint8_t *rowData)
    {
        _flash->read(l * _frame_size, rowData, _frame_size);
    }

    void StoreRowInFlash(uint8_t *rowData, int index)
    {
        _flash->write(_frame_size * index, rowData, _frame_size);
    }

    uint16_t FindMissingFrameIndex(uint16_t x)
    {
        uint16_t i;
        for (i = 0; i < _frame_count; i++)
        {
            if (missingFrameIndex[i] == (x + 1))
            {
                return (i);
            }
        }
        return (0);
    }

    void FindMissingReceiveFrame(uint16_t frameCounter)
    {
        uint16_t q;

        for (q = lastReceiveFrameCnt; q < (frameCounter - 1); q++)
        {
            if (q < _frame_count)
            {
                numberOfLoosingFrame++;
                missingFrameIndex[q] = numberOfLoosingFrame;
            }
        }
        if (q < _frame_count)
        {
            lastReceiveFrameCnt = frameCounter;
        }
        else
        {
            lastReceiveFrameCnt = _frame_count + 1;
        }
    }

    /*!
    * \brief	Function to xor two line of data
    *
    * \param	[IN] dataL1 and dataL2
    * \param    [IN] size : number of Bytes in dataL1
    * \param	[OUT] xor(dataL1,dataL2) in dataL1
    */
    void XorLineData(uint8_t *dataL1, uint8_t *dataL2, int size)
    {
        int i;
        uint8_t *dataTemp = (uint8_t *)malloc(size);
        if (!dataTemp)
            printf("[FragmentationMath] XorLineData malloc out of memory!\n");

        for (i = 0; i < size; i++)
        {
            dataTemp[i] = dataL1[i] ^ dataL2[i];
        }
        for (i = 0; i < size; i++)
        {
            dataL1[i] = dataTemp[i];
        }
        free(dataTemp);
    }

    /*!
    * \brief	Function to xor two line of data
    *
    * \param	[IN] dataL1 and dataL2
    * \param    [IN] size : number of bool in dataL1
    * \param	[OUT] xor(dataL1,dataL2) store in dataL1
    */
    void XorLineBool(bool *dataL1, bool *dataL2, int size)
    {
        int i;
        bool *dataTemp = (bool *)malloc(size);
        if (!dataTemp)
            printf("[FragmentationMath] XorLineBool malloc failed\n");

        for (i = 0; i < size; i++)
        {
            dataTemp[i] = dataL1[i] ^ dataL2[i];
        }
        for (i = 0; i < size; i++)
        {
            dataL1[i] = dataTemp[i];
        }

        free(dataTemp);
    }

    /*!
    * \brief	Function to find the first one in a bolean vector
    *
    * \param	[IN] bool vector and size of vector
    * \param	[OUT] the position of the first one in the row vector
    */

    int FindFirstOne(bool *boolData, int size)
    {
        int i;
        for (i = 0; i < size; i++)
        {
            if (boolData[i] == 1)
            {
                return i;
            }
        }
        return 0;
    }

    /*!
    * \brief	Function to test if a vector is null
    *
    * \param	[IN] bool vector and size of vector
    * \param	[OUT] bool : true if vector is null
    */
    bool VectorIsNull(bool *boolData, int size)
    {
        int i;
        for (i = 0; i < size; i++)
        {
            if (boolData[i] == 1)
            {
                return false;
            }
        }
        return true;
    }

    /*!
 * \brief	Function extact a row from the binary matrix and expand to a bool vector
 *
 * \param	[IN] row number
 * \param	[IN] bool vector, number of Bits in one row
 */
    void ExtractLineFromBinaryMatrix(bool *boolVector, int rownumber, int numberOfBit)
    {
        int i;
        int findByte;
        int findBitInByte;
        if (rownumber > 0)
        {
            findByte = (rownumber * numberOfBit - ((rownumber * (rownumber - 1)) / 2)) / 8;
            findBitInByte = (rownumber * numberOfBit - ((rownumber * (rownumber - 1)) / 2)) % 8;
        }
        else
        {
            findByte = 0;
            findBitInByte = 0;
        }
        if (rownumber > 0)
        {
            for (i = 0; i < rownumber; i++)
            {
                boolVector[i] = 0;
            }
        }
        for (i = rownumber; i < numberOfBit; i++)
        {
            boolVector[i] = (matrixM2B[findByte] >> (7 - findBitInByte)) & 0x01;
            findBitInByte++;
            if (findBitInByte == 8)
            {
                findBitInByte = 0;
                findByte++;
            }
        }
    }

    /*!
 * \brief	Function Collapse and Push  a row vector to the binary matrix
 *
 * \param	[IN] row number
 * \param	[IN] bool vector, number of Bits in one row
 */
    void PushLineToBinaryMatrix(bool *boolVector, int rownumber, int numberOfBit)
    {
        int i;
        int findByte;
        int findBitInByte;
        if (rownumber > 0)
        {
            findByte = (rownumber * numberOfBit - ((rownumber * (rownumber - 1)) / 2)) / 8;
            findBitInByte = (rownumber * numberOfBit - ((rownumber * (rownumber - 1)) / 2)) % 8;
        }
        else
        {
            findByte = 0;
            findBitInByte = 0;
        }
        for (i = rownumber; i < numberOfBit; i++)
        {
            if (boolVector[i] == 1)
            {
                matrixM2B[findByte] = (matrixM2B[findByte] & (0xFF - (1 << (7 - findBitInByte)))) + (1 << (7 - findBitInByte));
            }
            else
            {
                matrixM2B[findByte] = (matrixM2B[findByte] & (0xFF - (1 << (7 - findBitInByte))));
            }
            findBitInByte++;
            if (findBitInByte == 8)
            {
                findBitInByte = 0;
                findByte++;
            }
        }
    }

    /*!
 * \brief	Function to calculate a certain row from the parity check matrix
 *
 * \param	[IN] i - the index of the row to be calculated
 * \param	[IN] M - the size of the row to be calculted, the number of uncoded fragments used in the scheme,matrixRow - pointer to the boolean array
 * \param	[OUT] void
 */
    void FragmentationGetParityMatrixRow(int N, int M, bool *matrixRow)
    {

        int i;
        int m;
        int x;
        int nbCoeff = 0;
        int r;
        if (IsPowerOfTwo(M))
        {
            m = 1;
        }
        else
        {
            m = 0;
        }
        x = 1 + (1001 * N);
        for (i = 0; i < M; i++)
        {
            matrixRow[i] = 0;
        }
        while (nbCoeff < (M >> 1))
        {
            r = 1 << 16;
            while (r >= M)
            {
                x = FragmentationPrbs23(x);
                r = x % (M + m);
            }
            matrixRow[r] = 1;
            nbCoeff += 1;
        }
    }

    /*!
 * \brief	Pseudo random number generator : prbs23
 * \param	[IN] x - the input of the prbs23 generator
 */
    int FragmentationPrbs23(int x)
    {
        int b0 = x & 1;
        int b1 = (x & 0x20) >> 5;
        x = (int)floor((double)x / 2) + ((b0 ^ b1) << 22);
        return x;
    }
    /*!
 * \brief	Function to determine whether a frame is a fragmentation command or fragmentation content
 *
 * \param	[IN]  input variable to be tested
 * \param	[OUT] return true if x is a power of two
 */
    bool IsPowerOfTwo(unsigned int x)
    {
        int i;
        int bi;
        int sumBit = 0;
        for (i = 0; i < 32; i++)
        {
            bi = 1 << i;
            sumBit += (x & bi) >> i;
        }
        if (sumBit == 1)
        {
            return (true);
        }
        else
        {
            return (false);
        }
    }

    IFlash *_flash;
    uint16_t _frame_count;
    uint8_t _frame_size;
    uint16_t _redundancy_max;

    uint8_t *matrixM2B;
    uint16_t *missingFrameIndex;

    bool *matrixRow;
    uint8_t *matrixDataTemp;
    bool *dataTempVector;
    bool *dataTempVector2;
    bool *s;

    int numberOfLoosingFrame;
    int lastReceiveFrameCnt;
};

#endif // _MBEDFRAG_FRAGMENTATION_MATH_H
