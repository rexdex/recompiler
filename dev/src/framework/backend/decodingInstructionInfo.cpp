#include "build.h"
#include "internalUtils.h"
#include "internalParser.inl"
#include "platformDefinition.h"
#include "platformCPU.h"
#include "decodingInstructionInfo.h"
#include "traceData.h"
#include "traceUtils.h"

namespace decoding
{

	InstructionExtendedInfo::InstructionExtendedInfo()
		: m_codeAddress(0)
		, m_codeFlags(0)
		, m_memoryAddressBase(0)
		, m_memoryAddressOffset(0)
		, m_memoryAddressIndex(0)
		, m_memoryAddressScale(1)
		, m_memoryFlags(0)
		, m_memorySize(0)
		, m_registersDependenciesCount(0)
		, m_registersModifiedCount(0)
		, m_branchTargetAddress(0)
		, m_branchTargetReg(NULL)
	{}

	bool InstructionExtendedInfo::AddRegister(const platform::CPURegister* reg, const ERegDependencyMode mode)
	{
		if (mode & eReg_Dependency)
			if ( !AddRegisterDependency(reg) )
				return false;

		if (mode & eReg_Output)
			if ( !AddRegisterOutput(reg) )
				return false;

		return true;
	}

	bool InstructionExtendedInfo::AddRegisterDependency(const platform::CPURegister* reg)
	{
		if (!reg)
			return false;

		for (uint32 i=0; i<m_registersDependenciesCount; ++i)
		{
			if (m_registersDependencies[i] == reg)
				return true;
		}

		if (m_registersDependenciesCount == MAX_REGISTERS)
			return false;

		m_registersDependencies[ m_registersDependenciesCount ] = reg;
		m_registersDependenciesCount += 1;
		return true;
	}

	bool InstructionExtendedInfo::AddRegisterOutput(const platform::CPURegister* reg, const bool modifiesWholeRegister)
	{
		if (!reg)
			return false;

		for (uint32 i=0; i<m_registersModifiedCount; ++i)
		{
			if (m_registersModified[i] == reg)
				return true;
		}

		if (m_registersModifiedCount == MAX_REGISTERS)
			return false;

		m_registersModified[ m_registersModifiedCount ] = reg;
		m_registersModifiedCount += 1;
		return true;
	}

	bool InstructionExtendedInfo::ComputeBranchTargetAddress(const trace::DataFrame& data, uint64& outAddress) const
	{
		// branch target is taken from register
		if (m_branchTargetReg)
		{
			outAddress = trace::GetRegisterValueInteger(m_branchTargetReg, data);
			return true;
		}

		// branch target is literal
		if (m_branchTargetAddress)
		{
			outAddress = m_branchTargetAddress;
			return true;
		}

		// nothing
		return false;
	}

	bool InstructionExtendedInfo::ComputeMemoryAddress(const trace::DataFrame& data, uint64& outAddress) const
	{
		if (0 == (m_memoryFlags & (eMemoryFlags_Read | eMemoryFlags_Write | eMemoryFlags_Touch)))
			return false;

		uint64 addr = m_memoryAddressOffset;

		// base address
		if (m_memoryAddressBase)
		{
			addr += trace::GetRegisterValueInteger(m_memoryAddressBase, data, true);
		}

		// index address
		if (m_memoryAddressIndex)
		{
			const auto indexVal = trace::GetRegisterValueInteger(m_memoryAddressIndex, data, true);
			addr += indexVal * m_memoryAddressScale;
		}

		// valid address computed
		outAddress = (uint32) addr; // TODO: the 32 bit clamp is tempshit
		return true;
	}

} // decoding