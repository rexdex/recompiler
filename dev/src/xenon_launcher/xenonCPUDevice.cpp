#include "build.h"
#include "xenonCPUDevice.h"

#define ADD_REG( name, bitSize, regType ) AddRootRegister( regType, #name, bitSize, (uint32)&regs->##name )
#define ADD_CHILD_REG( parent, name, bitSize, bitOffset, regType ) AddChildRegister( regType, #parent, #name, bitSize, bitOffset )

namespace xenon
{

	//----

	CPU_RegisterBankInfo::CPU_RegisterBankInfo()
		: runtime::RegisterBankInfo()
	{
		Setup("Xbox360", "ppc");

		cpu::CpuRegs* regs = (cpu::CpuRegs*)0;

		ADD_REG(LR, 32, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(CTR, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(MSR, 64, runtime::RegisterInfo::RegisterType::Flags);
		ADD_REG(CR, 32, runtime::RegisterInfo::RegisterType::Flags);

		ADD_CHILD_REG(CR, CR0, 4, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR, CR1, 4, 4, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR, CR2, 4, 8, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR, CR3, 4, 12, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR, CR4, 4, 16, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR, CR5, 4, 20, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR, CR6, 4, 24, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR, CR7, 4, 28, runtime::RegisterInfo::RegisterType::Flags);

		ADD_CHILD_REG(CR0, CR0_LT, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR0, CR0_GT, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR0, CR0_EQ, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR0, CR0_SO, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR1, CR1_LT, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR1, CR1_GT, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR1, CR1_EQ, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR1, CR1_SO, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR2, CR2_LT, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR2, CR2_GT, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR2, CR2_EQ, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR2, CR2_SO, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR3, CR3_LT, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR3, CR3_GT, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR3, CR3_EQ, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR3, CR3_SO, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR4, CR4_LT, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR4, CR4_GT, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR4, CR4_EQ, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR4, CR4_SO, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR5, CR5_LT, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR5, CR5_GT, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR5, CR5_EQ, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR5, CR5_SO, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR6, CR6_LT, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR6, CR6_GT, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR6, CR6_EQ, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR6, CR6_SO, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR7, CR7_LT, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR7, CR7_GT, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR7, CR7_EQ, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(CR7, CR7_SO, 1, 3, runtime::RegisterInfo::RegisterType::Flags);

		ADD_REG(XER, 64, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(XER, XER_CA, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(XER, XER_OV, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(XER, XER_SO, 1, 2, runtime::RegisterInfo::RegisterType::Flags);

		ADD_REG(FPSCR, 32, runtime::RegisterInfo::RegisterType::Flags);

		ADD_CHILD_REG(FPSCR, FPSCR0, 4, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR, FPSCR1, 4, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR, FPSCR2, 4, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR, FPSCR3, 4, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR, FPSCR4, 4, 4, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR, FPSCR5, 4, 5, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR, FPSCR6, 4, 6, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR, FPSCR7, 4, 7, runtime::RegisterInfo::RegisterType::Flags);

		ADD_CHILD_REG(FPSCR0, FPSCR0_FX, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR0, FPSCR0_FEX, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR0, FPSCR0_VX, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR0, FPSCR0_OX, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR1, FPSCR1_UX, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR1, FPSCR1_ZX, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR1, FPSCR1_XX, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR1, FPSCR1_VXSNAN, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR2, FPSCR2_VXISI, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR2, FPSCR2_VXIDI, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR2, FPSCR2_VXZDZ, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR2, FPSCR2_VXIMZ, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR3, FPSCR3_VXVC, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR3, FPSCR3_FR, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR3, FPSCR3_FI, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR3, FPSCR3_C, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR4, FPSCR4_FL, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR4, FPSCR4_FG, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR4, FPSCR4_FE, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR4, FPSCR4_FU, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR5, FPSCR5_RES, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR5, FPSCR5_VXSOFT, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR5, FPSCR5_VXSQRT, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR5, FPSCR5_VXCVI, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR6, FPSCR6_VE, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR6, FPSCR6_VO, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR6, FPSCR6_UE, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR6, FPSCR6_ZE, 1, 3, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR7, FPSCR7_XE, 1, 0, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR7, FPSCR7_NI, 1, 1, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR7, FPSCR7_RN0, 1, 2, runtime::RegisterInfo::RegisterType::Flags);
		ADD_CHILD_REG(FPSCR7, FPSCR7_RN1, 1, 3, runtime::RegisterInfo::RegisterType::Flags);

		ADD_REG(R0, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R1, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R2, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R3, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R4, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R5, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R6, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R7, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R8, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R9, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R10, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R11, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R12, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R13, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R14, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R15, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R16, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R17, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R18, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R19, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R20, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R21, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R22, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R23, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R24, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R25, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R26, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R27, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R28, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R29, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R30, 64, runtime::RegisterInfo::RegisterType::Fixed);
		ADD_REG(R31, 64, runtime::RegisterInfo::RegisterType::Fixed);

		ADD_REG(FR0, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR1, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR2, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR3, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR4, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR5, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR6, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR7, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR8, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR9, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR10, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR11, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR12, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR13, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR14, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR15, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR16, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR17, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR18, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR19, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR20, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR21, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR22, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR23, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR24, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR25, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR26, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR27, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR28, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR29, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR30, 64, runtime::RegisterInfo::RegisterType::Float);
		ADD_REG(FR31, 64, runtime::RegisterInfo::RegisterType::Float);

		ADD_REG(VR0, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR1, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR2, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR3, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR4, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR5, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR6, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR7, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR8, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR9, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR10, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR11, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR12, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR13, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR14, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR15, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR16, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR17, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR18, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR19, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR20, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR21, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR22, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR23, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR24, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR25, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR26, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR27, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR28, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR29, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR30, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR31, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR32, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR33, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR34, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR35, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR36, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR37, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR38, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR39, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR40, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR41, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR42, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR43, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR44, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR45, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR46, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR47, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR48, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR49, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR50, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR51, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR52, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR53, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR54, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR55, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR56, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR57, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR58, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR59, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR60, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR61, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR62, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR63, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR64, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR65, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR66, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR67, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR68, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR69, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR70, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR71, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR72, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR73, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR74, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR75, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR76, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR77, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR78, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR79, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR80, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR81, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR82, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR83, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR84, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR85, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR86, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR87, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR88, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR89, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR90, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR91, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR92, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR93, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR94, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR95, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR96, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR97, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR98, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR99, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR100, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR101, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR102, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR103, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR104, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR105, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR106, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR107, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR108, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR109, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR110, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR111, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR112, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR113, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR114, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR115, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR116, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR117, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR118, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR119, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR120, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR121, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR122, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR123, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR124, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR125, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR126, 128, runtime::RegisterInfo::RegisterType::Wide);
		ADD_REG(VR127, 128, runtime::RegisterInfo::RegisterType::Wide);
	}

	CPU_RegisterBankInfo& CPU_RegisterBankInfo::GetInstance()
	{
		static CPU_RegisterBankInfo theInstance;
		return theInstance;
	}

	//----


	CPU_PowerPC::CPU_PowerPC()
		: IDeviceCPU()
	{}

	CPU_PowerPC& CPU_PowerPC::GetInstance()
	{
		static CPU_PowerPC theInstance;
		return theInstance;
	}

} // xenon
