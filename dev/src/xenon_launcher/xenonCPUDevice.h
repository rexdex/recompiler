#pragma once
#include "../host_core/runtimeCPU.h"

namespace xenon
{
	/// cpu bank info
	class CPU_RegisterBankInfo : public runtime::RegisterBankInfo
	{
	public:
		static CPU_RegisterBankInfo& GetInstance();

	private:
		CPU_RegisterBankInfo();
	};

	/// cpu description, never freed
	class CPU_PowerPC : public runtime::IDeviceCPU
	{
	public:
		static CPU_PowerPC& GetInstance();

		// runtime::IDeviceCPU interface
		virtual const runtime::RegisterBankInfo& GetRegisterBankInfo() const override final { return CPU_RegisterBankInfo::GetInstance(); }

		// runtime::IDevice interface
		virtual const char* GetName() const override final { return "PowerPC CPU";  }

	private:
		CPU_PowerPC();
	};

} // xenon