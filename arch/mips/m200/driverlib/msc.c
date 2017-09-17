//*****************************************************************************
//
// msc.c - Driver for the MSC(MMC/SD Controller).
//
// Copyright (c) 2017 Harvis Wang.  All rights reserved.
// Software License Agreement
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
//
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the
//   distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#include "../inc/hw_msc.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_types.h"
#include "debug.h"
#include "msc.h"
#include "intc.h"

//*****************************************************************************
//
// A mapping of MSC base address to interupt number.
//
//*****************************************************************************
static const unsigned long g_ppulMSCIntMap[][2] =
{
    { MSC0_BASE, IRQ_NO_MSC0 },
    { MSC1_BASE, IRQ_NO_MSC1 },
    { MSC2_BASE, IRQ_NO_MSC2 },
};

//*****************************************************************************
//
//! \internal
//! Checks a MSC base address.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function determines if a MSC port base address is valid.
//!
//! \return Returns \b true if the base address is valid and \b false
//! otherwise.
//*****************************************************************************
#ifdef DEBUG
static tBoolean
MSCBaseValid(unsigned long ulBase)
{
    return((ulBase == MSC0_BASE) || (ulBase == MSC1_BASE) ||
           (ulBase == MSC2_BASE));
}
#endif

//*****************************************************************************
//
//! Print all register value.
//!
//! \param ulBase is the base address of the MSC.
//! \param print is a function pointer like printf.
//!
//! This function print value of all register in a MSC.
//!
//! \return none
//
//*****************************************************************************
void
MSCRegisterDump(unsigned long ulBase, int (*print)(const char *format, ...))
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    REGH_PRINT(MSC_O_CTRL,  ulBase, print);
    REG_PRINT(MSC_O_STAT,   ulBase, print);
    REGH_PRINT(MSC_O_CLKRT, ulBase, print);
    REG_PRINT(MSC_O_CMDAT,  ulBase, print);
    REGH_PRINT(MSC_O_RESTO, ulBase, print);
    REG_PRINT(MSC_O_RDTO,   ulBase, print);
    REGH_PRINT(MSC_O_BLKEN, ulBase, print);
    REGH_PRINT(MSC_O_NOB,   ulBase, print);
    REG_PRINT(MSC_O_SNOB,   ulBase, print);
    REG_PRINT(MSC_O_IMASK,  ulBase, print);
    REG_PRINT(MSC_O_IFLG,   ulBase, print);
    REGB_PRINT(MSC_O_CMD,   ulBase, print);
    REG_PRINT(MSC_O_ARG,    ulBase, print);
    REGH_PRINT(MSC_O_RES,   ulBase, print);
    REG_PRINT(MSC_O_RXFIFO, ulBase, print);
    REG_PRINT(MSC_O_TXFIFO, ulBase, print);
    REG_PRINT(MSC_O_LPM,    ulBase, print);
    REG_PRINT(MSC_O_DMAC,   ulBase, print);
    REG_PRINT(MSC_O_DMANDA, ulBase, print);
    REG_PRINT(MSC_O_DMADA,  ulBase, print);
    REG_PRINT(MSC_O_DMALEN, ulBase, print);
    REG_PRINT(MSC_O_DMACMD, ulBase, print);
    REG_PRINT(MSC_O_CTRL2, ulBase, print);
    REG_PRINT(MSC_O_RTCNT, ulBase, print);
}

//*****************************************************************************
//
//! Set bus width.
//!
//! \param ulBase is the base address of the MSC.
//! \param ulBusWidth is the bus width.
//!
//! This function set the bus width of a MSC.
//!
//! \return true if set operation successed, false others.
//
//*****************************************************************************
tBoolean
MSCBusWidthSet(unsigned long ulBase, unsigned long ulBusWidth)
{
    unsigned long ulReg;

    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));
    ASSERT((ulBusWidth == MSC_BUSWIDTH_1BIT) ||
           (ulBusWidth == MSC_BUSWIDTH_4BIT) ||
           (ulBusWidth == MSC_BUSWIDTH_8BIT));

    switch (ulBusWidth) {
        case MSC_BUSWIDTH_1BIT:
            ulReg = HWREG(ulBase + MSC_O_CMDAT);
            HWREG(ulBase + MSC_O_CMDAT) = (ulReg & (~MSC_CMDAT_BUSWIDTH)) | MSC_CMDAT_BUSWIDTH_1;
            return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_BUSWIDTH) == MSC_CMDAT_BUSWIDTH_1);
        case MSC_BUSWIDTH_4BIT:
            ulReg = HWREG(ulBase + MSC_O_CMDAT);
            HWREG(ulBase + MSC_O_CMDAT) = (ulReg & (~MSC_CMDAT_BUSWIDTH)) | MSC_CMDAT_BUSWIDTH_4;
            return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_BUSWIDTH) == MSC_CMDAT_BUSWIDTH_4);
        case MSC_BUSWIDTH_8BIT:
            ulReg = HWREG(ulBase + MSC_O_CMDAT);
            HWREG(ulBase + MSC_O_CMDAT) = (ulReg & (~MSC_CMDAT_BUSWIDTH)) | MSC_CMDAT_BUSWIDTH_8;
            return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_BUSWIDTH) == MSC_CMDAT_BUSWIDTH_8);
        default:
            return false;
    }
}

//*****************************************************************************
//
//! Enable SDIO interrupt.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function enable the interrupt of a MSC SDIO.
//!
//! \return true if the enable operation successed, false others.
//
//*****************************************************************************
tBoolean
MSCSDIOInterruptEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_IMASK) &= ~MSC_IMASK_SDIO;
    return((HWREG(ulBase + MSC_O_IMASK) & MSC_IMASK_SDIO) != MSC_IMASK_SDIO);
}

//*****************************************************************************
//
//! Disable SDIO interrupt.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function disable the interrupt of a MSC SDIO.
//!
//! \return true if the disable operation successed, false others.
//
//*****************************************************************************
tBoolean
MSCSDIOInterruptDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_IMASK) |= MSC_IMASK_SDIO;
    return((HWREG(ulBase + MSC_O_IMASK) & MSC_IMASK_SDIO) == MSC_IMASK_SDIO);
}

//*****************************************************************************
//
//! MSC read time out limit set.
//!
//! \param ulBase is the base address of the MSC.
//! \param ulTimeOutClks is the unmber of MSC clock units.
//!
//! This function set the read time out of a MSC.
//!
//! \return true if the read time out set operation successed, false others.
//
//*****************************************************************************
tBoolean
MSCReadTimeOutSet(unsigned long ulBase, unsigned long ulTimeOutClks)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_RDTO) = ulTimeOutClks & MSC_RDTO_READTO;
    return((HWREG(ulBase + MSC_O_RDTO) & MSC_RDTO_READTO) == (ulTimeOutClks & MSC_RDTO_READTO));
}

//*****************************************************************************
//
//! MSC read time out limit get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get the read time out of a MSC.
//!
//! \return the unmber of MSC clock units.
//
//*****************************************************************************
unsigned long
MSCReadTimeOutGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    return(HWREG(ulBase + MSC_O_RDTO) & MSC_RDTO_READTO);
}

//*****************************************************************************
//
//! MSC block count set.
//!
//! \param ulBase is the base address of the MSC.
//! \param ulBlockCount is the number of requested blocks.
//!
//! This function set the number of blocks in a MSC request.
//!
//! \return true if the unmber of block is set ok, false others.
//
//*****************************************************************************
tBoolean
MSCBlockCountSet(unsigned long ulBase, unsigned long ulBlockCount)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_NOB) = ulBlockCount & MSC_NOB_NOB;
    return((HWREG(ulBase + MSC_O_NOB) & MSC_NOB_NOB) == (ulBlockCount & MSC_NOB_NOB));
}

//*****************************************************************************
//
//! MSC block count get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get the block number of a MSC request.
//!
//! \return the number of requested blocks.
//
//*****************************************************************************
unsigned long
MSCBlockCountGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    return(HWREG(ulBase + MSC_O_NOB) & MSC_NOB_NOB);
}

//*****************************************************************************
//
//! MSC block size set.
//!
//! \param ulBase is the base address of the MSC.
//! \param ulBlockSize is the size of one block.
//!
//! This function set the the size of one block.
//!
//! \return true if the block size is set ok, false others.
//
//*****************************************************************************
tBoolean
MSCBlockSizeSet(unsigned long ulBase, unsigned long ulBlockSize)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREGH(ulBase + MSC_O_BLKEN) = (ulBlockSize & MSC_BLKEN_BLKEN);
    return((HWREGH(ulBase + MSC_O_BLKEN) & MSC_BLKEN_BLKEN) == (ulBlockSize & MSC_BLKEN_BLKEN));
}

//*****************************************************************************
//
//! MSC block size get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get the the size of one block.
//!
//! \return true the block size.
//
//*****************************************************************************
unsigned long
MSCBlockSizeGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    return(HWREGH(ulBase + MSC_O_BLKEN) & MSC_BLKEN_BLKEN);
}

//*****************************************************************************
//
//! MSC read timeout set.
//!
//! \param ulBase is the base address of the MSC.
//! \param ulReadTimeout is the read timeout, in MSC_CLK units.
//!
//! This function set the the read timeout.
//!
//! \return true if the read timeout is set ok, false others.
//
//*****************************************************************************
tBoolean
MSCReadTimeoutSet(unsigned long ulBase, unsigned long ulReadTimeout)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_RDTO) = (ulReadTimeout & MSC_RDTO_READTO);
    return((HWREG(ulBase + MSC_O_RDTO) & MSC_RDTO_READTO) == (ulReadTimeout & MSC_RDTO_READTO));
}

//*****************************************************************************
//
//! MSC read timeout get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get the the read timeout.
//!
//! \return the read timeout(in MSC_CLK units).
//
//*****************************************************************************
unsigned long
MSCReadTimeoutGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    return(HWREG(ulBase + MSC_O_RDTO) & MSC_RDTO_READTO);
}

//*****************************************************************************
//
//! MSC stream block mode enable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function enable stream block mode in data transfer.
//!
//! \return true if stream block mode is enabled.
//
//*****************************************************************************
tBoolean
MSCStreamModeEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_CMDAT) |= MSC_CMDAT_STREAMBLOCK;
    return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_STREAMBLOCK) == MSC_CMDAT_STREAMBLOCK);
}

//*****************************************************************************
//
//! MSC stream block mode disable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function disable stream block mode(normal block mode) in data transfer.
//!
//! \return true if stream block mode is disabled.
//
//*****************************************************************************
tBoolean
MSCStreamModeDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_CMDAT) &= ~MSC_CMDAT_STREAMBLOCK;
    return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_STREAMBLOCK) != MSC_CMDAT_STREAMBLOCK);
}

//*****************************************************************************
//
//! MSC response format set.
//!
//! \param ulBase is the base address of the MSC.
//! \param ulResponseFormat is the response format.
//!
//! This function set response format.
//!
//! \return true if the response format set operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCResponseFormatSet(unsigned long ulBase, unsigned long ulResponseFormat)
{
    unsigned long ulReg;

    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));
    ASSERT((ulResponseFormat == MSC_RESPONSE_NONE)  ||
           (ulResponseFormat == MSC_RESPONSE_R1R1B) ||
           (ulResponseFormat == MSC_RESPONSE_R2)    ||
           (ulResponseFormat == MSC_RESPONSE_R3)    ||
           (ulResponseFormat == MSC_RESPONSE_R4)    ||
           (ulResponseFormat == MSC_RESPONSE_R5)    ||
           (ulResponseFormat == MSC_RESPONSE_R6)    ||
           (ulResponseFormat == MSC_RESPONSE_R7));

    switch(ulResponseFormat) {
    case MSC_RESPONSE_NONE:
        ulReg = HWREG(ulBase + MSC_O_CMDAT);
        ulReg = (ulReg & ~MSC_CMDAT_RESPONSEFORMAT) | MSC_RESPONSE_NONE;
        HWREG(ulBase + MSC_O_CMDAT) = ulReg;
        return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_RESPONSEFORMAT) == MSC_RESPONSE_NONE);
    case MSC_RESPONSE_R1R1B:
        ulReg = HWREG(ulBase + MSC_O_CMDAT);
        ulReg = (ulReg & ~MSC_CMDAT_RESPONSEFORMAT) | MSC_RESPONSE_R1R1B;
        HWREG(ulBase + MSC_O_CMDAT) = ulReg;
        return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_RESPONSEFORMAT) == MSC_RESPONSE_R1R1B);
    case MSC_RESPONSE_R2:
        ulReg = HWREG(ulBase + MSC_O_CMDAT);
        ulReg = (ulReg & ~MSC_CMDAT_RESPONSEFORMAT) | MSC_RESPONSE_R2;
        HWREG(ulBase + MSC_O_CMDAT) = ulReg;
        return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_RESPONSEFORMAT) == MSC_RESPONSE_R2);
    case MSC_RESPONSE_R3:
        ulReg = HWREG(ulBase + MSC_O_CMDAT);
        ulReg = (ulReg & ~MSC_CMDAT_RESPONSEFORMAT) | MSC_RESPONSE_R3;
        HWREG(ulBase + MSC_O_CMDAT) = ulReg;
        return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_RESPONSEFORMAT) == MSC_RESPONSE_R3);
    case MSC_RESPONSE_R4:
        ulReg = HWREG(ulBase + MSC_O_CMDAT);
        ulReg = (ulReg & ~MSC_CMDAT_RESPONSEFORMAT) | MSC_RESPONSE_R4;
        HWREG(ulBase + MSC_O_CMDAT) = ulReg;
        return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_RESPONSEFORMAT) == MSC_RESPONSE_R4);
    case MSC_RESPONSE_R5:
        ulReg = HWREG(ulBase + MSC_O_CMDAT);
        ulReg = (ulReg & ~MSC_CMDAT_RESPONSEFORMAT) | MSC_RESPONSE_R5;
        HWREG(ulBase + MSC_O_CMDAT) = ulReg;
        return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_RESPONSEFORMAT) == MSC_RESPONSE_R5);
    case MSC_RESPONSE_R6:
        ulReg = HWREG(ulBase + MSC_O_CMDAT);
        ulReg = (ulReg & ~MSC_CMDAT_RESPONSEFORMAT) | MSC_RESPONSE_R6;
        HWREG(ulBase + MSC_O_CMDAT) = ulReg;
        return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_RESPONSEFORMAT) == MSC_RESPONSE_R6);
    case MSC_RESPONSE_R7:
        ulReg = HWREG(ulBase + MSC_O_CMDAT);
        ulReg = (ulReg & ~MSC_CMDAT_RESPONSEFORMAT) | MSC_RESPONSE_R7;
        HWREG(ulBase + MSC_O_CMDAT) = ulReg;
        return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_RESPONSEFORMAT) == MSC_RESPONSE_R7);
    default:
        ASSERT(false);
        return(false);
    }
}

//*****************************************************************************
//
//! MSC command busy signal set.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function set busy.
//!
//! \return true if the busy set operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCBusySet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_CMDAT) |= MSC_CMDAT_BUSY;
    return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_BUSY) == MSC_CMDAT_BUSY);
}

//*****************************************************************************
//
//! MSC command busy signal clear.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function clear busy.
//!
//! \return true if the busy clear operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCBusyClear(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_CMDAT) &= ~MSC_CMDAT_BUSY;
    return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_BUSY) != MSC_CMDAT_BUSY);
}

//*****************************************************************************
//
//! MSC AutoCMD12 enable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function enable Auto CMD12.
//!
//! \return true if the Auto CMD12 enable operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCAutoCMD12Enable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_CMDAT) |= MSC_CMDAT_AUTOCMD12;
    return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_AUTOCMD12) == MSC_CMDAT_AUTOCMD12);
}

//*****************************************************************************
//
//! MSC AutoCMD12 disable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function disable Auto CMD12.
//!
//! \return true if the Auto CMD12 disable operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCAutoCMD12Disable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_CMDAT) &= ~MSC_CMDAT_AUTOCMD12;
    return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_AUTOCMD12) != MSC_CMDAT_AUTOCMD12);
}

//*****************************************************************************
//
//! MSC command index set.
//!
//! \param ulBase is the base address of the MSC.
//! \param ulCommandIndex is the the command index(opcode).
//!
//! This function set command index.
//!
//! \return true if the opcode set operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCCommandIndexSet(unsigned long ulBase, unsigned long ulCommandIndex)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREGB(ulBase + MSC_O_CMD) = (ulCommandIndex & MSC_CMD_CMDINDEX);
    return((HWREGB(ulBase + MSC_O_CMD) & MSC_CMD_CMDINDEX) == (ulCommandIndex & MSC_CMD_CMDINDEX));
}

//*****************************************************************************
//
//! MSC command argument set.
//!
//! \param ulBase is the base address of the MSC.
//! \param ulCommandArgument is the the command argument.
//!
//! This function set command argument.
//!
//! \return true if the command argument set operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCCommandArgumentSet(unsigned long ulBase, unsigned long ulCommandArgument)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_ARG) = (ulCommandArgument & MSC_ARG_ARG);
    return((HWREG(ulBase + MSC_O_ARG) & MSC_ARG_ARG) == (ulCommandArgument & MSC_ARG_ARG));
}

//*****************************************************************************
//
//! MSC new operation start.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function start a new operation.
//!
//! \return true if the new operation start operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCNewOperationStart(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_CTRL) |= MSC_CTRL_STARTOP;
    return((HWREG(ulBase + MSC_O_CTRL) & MSC_CTRL_STARTOP) == MSC_CTRL_STARTOP);
}

//*****************************************************************************
//
//! MSC end command response interrupt enable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function enable end command response interrupt.
//!
//! \return true if the end command response interrupt enable operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCEndCommandResponseEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_IMASK) &= ~MSC_IMASK_ENDCMDRES;
    return((HWREG(ulBase + MSC_O_IMASK) & MSC_IMASK_ENDCMDRES) != MSC_IMASK_ENDCMDRES);
}

//*****************************************************************************
//
//! MSC end command response interrupt disable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function disable end command response interrupt.
//!
//! \return true if the end command response interrupt disable operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCEndCommandResponseDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_IMASK) |= MSC_IMASK_ENDCMDRES;
    return((HWREG(ulBase + MSC_O_IMASK) & MSC_IMASK_ENDCMDRES) == MSC_IMASK_ENDCMDRES);
}

//*****************************************************************************
//
//! MSC end command response interrupt flag clear.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function clear end command response interrupt flag.
//!
//! \return true if the end command response interrupt flag clear operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCEndCommandResponseFlagClear(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_IFLG) |= MSC_IFLG_ENDCMDRES;
    return((HWREG(ulBase + MSC_O_IFLG) & MSC_IFLG_ENDCMDRES) != MSC_IFLG_ENDCMDRES);
}

//*****************************************************************************
//
//! MSC end command response interrupt flag get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get end command response interrupt flag's value.
//!
//! \return true if the end command response interrupt flag is set(by hardware), false others.
//
//*****************************************************************************
tBoolean
MSCEndCommandResponseFlagGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    return((HWREG(ulBase + MSC_O_IFLG) & MSC_IFLG_ENDCMDRES) == MSC_IFLG_ENDCMDRES);
}

//*****************************************************************************
//
//! MSC SDIO interrupt flag get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get SDIO interrupt flag's value.
//!
//! \return true if the SDIO interrupt flag is set(by hardware), false others.
//
//*****************************************************************************
tBoolean
MSCSDIOInterruptFlagGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    return((HWREG(ulBase + MSC_O_IFLG) & MSC_IFLG_SDIO) == MSC_IFLG_SDIO);
}

//*****************************************************************************
//
//! MSC command response get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get command response value.
//!
//! \return command response.
//
//*****************************************************************************
unsigned long
MSCCommandResponseGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));
    return(HWREGH(ulBase + MSC_O_RES) & MSC_RES_DATA);
}

//*****************************************************************************
//
//! MSC data transfer done interrupt enable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function enable data transfer done interrupt.
//!
//! \return true if enable is successed, false others.
//
//*****************************************************************************
tBoolean
MSCDataTransferDoneEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_IMASK) &= ~MSC_IMASK_DATATRANDONE;
    return((HWREG(ulBase + MSC_O_IMASK) & MSC_IMASK_DATATRANDONE) != MSC_IMASK_DATATRANDONE);
}

//*****************************************************************************
//
//! MSC data transfer done interrupt disable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function disable data transfer done interrupt.
//!
//! \return true if the interrupt disable operation is successed, false others.
//
//*****************************************************************************
tBoolean
MSCDataTransferDoneDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_IMASK) |= MSC_IMASK_DATATRANDONE;
    return((HWREG(ulBase + MSC_O_IMASK) & MSC_IMASK_DATATRANDONE) == MSC_IMASK_DATATRANDONE);
}

//*****************************************************************************
//
//! MSC data transfer done flag get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get data transfer done flag.
//!
//! \return the value of data transfer done flag.
//
//*****************************************************************************
tBoolean
MSCDataTransferDoneFlagClear(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_IFLG) |= MSC_IFLG_DATATRANDONE;
    return((HWREG(ulBase + MSC_O_IFLG) & MSC_IFLG_DATATRANDONE) != MSC_IFLG_DATATRANDONE);
}

//*****************************************************************************
//
//! MSC data transfer done flag get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get data transfer done flag.
//!
//! \return the value of data transfer done flag.
//
//*****************************************************************************
tBoolean
MSCDataTransferDoneFlagGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    return((HWREG(ulBase + MSC_O_IFLG) & MSC_IFLG_DATATRANDONE) == MSC_IFLG_DATATRANDONE);
}

//*****************************************************************************
//
//! MSC DMA Descriptor Address set.
//!
//! \param ulBase is the base address of the MSC.
//! \param ulDMADescriptorAddress is the DMA descriptor address.
//!
//! This function set DMA descriptor address.
//!
//! \return true if set DMA descriptor address operation successful, false others.
//
//*****************************************************************************
tBoolean
MSCDMADescriptorAddressSet(unsigned long ulBase, unsigned long ulDMADescriptorAddress)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_DMANDA) = ulDMADescriptorAddress & MSC_DMANDA_NDA;
    return((HWREG(ulBase + MSC_O_DMANDA) & MSC_DMANDA_NDA) == (ulDMADescriptorAddress & MSC_DMANDA_NDA));
}

//*****************************************************************************
//
//! MSC DMA Descriptor Address Get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get last DMA descriptor address.
//!
//! \return DMA descriptor address set before.
//
//*****************************************************************************
unsigned long
MSCDMADescriptorAddressGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    return(HWREG(ulBase + MSC_O_DMANDA) & MSC_DMANDA_NDA);
}

//*****************************************************************************
//
//! MSC DMA Data Done Enable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function enable MSC DMA Data Done interrupt.
//!
//! \return true if the DMA data done enable opreation is success, others false.
//
//*****************************************************************************
tBoolean
MSCDMADataDoneEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_IMASK) &= ~MSC_IMASK_DMADATADONE;
    return((HWREG(ulBase + MSC_O_IMASK) & MSC_IMASK_DMADATADONE) != MSC_IMASK_DMADATADONE);
}

//*****************************************************************************
//
//! MSC DMA Data Done Disable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function disable MSC DMA Data Done interrupt.
//!
//! \return true if the DMA data done disable opreation is success, others false.
//
//*****************************************************************************
tBoolean
MSCDMADataDoneDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_IMASK) |= MSC_IMASK_DMADATADONE;
    return((HWREG(ulBase + MSC_O_IMASK) & MSC_IMASK_DMADATADONE) == MSC_IMASK_DMADATADONE);
}

//*****************************************************************************
//
//! MSC DMA Data Done flag get.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function get the value of MSC DMA Data Done flag.
//!
//! \return the value of MSC DMA Data Done flag.
//
//*****************************************************************************
tBoolean
MSCDMADataDoneFlagGet(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    return((HWREG(ulBase + MSC_O_IFLG) & MSC_IFLG_DMADATADONE) == MSC_IFLG_DMADATADONE);
}

//*****************************************************************************
//
//! MSC DMA enable(start).
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function start DMA operation.
//!
//! \return true if DMA enable operation successed, false others.
//
//*****************************************************************************
tBoolean
MSCDMAEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_DMAC) |= MSC_DMAC_DMAEN;
    return((HWREG(ulBase + MSC_O_DMAC) & MSC_DMAC_DMAEN) == MSC_DMAC_DMAEN);
}

//*****************************************************************************
//
//! MSC DMA disable(stop).
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function stop DMA operation.
//!
//! \return true if DMA disable operation successed, false others.
//
//*****************************************************************************
tBoolean
MSCDMADisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_DMAC) &= ~MSC_DMAC_DMAEN;
    return((HWREG(ulBase + MSC_O_DMAC) & MSC_DMAC_DMAEN) != MSC_DMAC_DMAEN);
}

//*****************************************************************************
//
//! MSC data enable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function enable current command transfer data.
//!
//! \return true if data enable operation success, false others.
//
//*****************************************************************************
tBoolean
MSCDataEnable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_CMDAT) |= MSC_CMDAT_DATALEN;
    return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_DATALEN) == MSC_CMDAT_DATALEN);
}

//*****************************************************************************
//
//! MSC data disable.
//!
//! \param ulBase is the base address of the MSC.
//!
//! This function disable current command transfer data.
//!
//! \return true if data disable operation success, false others.
//
//*****************************************************************************
tBoolean
MSCDataDisable(unsigned long ulBase)
{
    //
    // Check the arguments.
    //
    ASSERT(MSCBaseValid(ulBase));

    HWREG(ulBase + MSC_O_CMDAT) &= ~MSC_CMDAT_DATALEN;
    return((HWREG(ulBase + MSC_O_CMDAT) & MSC_CMDAT_DATALEN) != MSC_CMDAT_DATALEN);
}
