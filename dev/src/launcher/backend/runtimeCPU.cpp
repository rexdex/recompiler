#include "build.h"
#include "runtimeCPU.h"

namespace runtime
{

	RegisterInfo::RegisterInfo(RegisterType type, const char* name, const uint32 index, const uint32 bitSize, const uint32 dataOffset)
		: m_type(type)
		, m_name(name)
		, m_index(index)
		, m_bitOffset(0)
		, m_bitSize(bitSize)
		, m_dataOffset(dataOffset)
		, m_parent(nullptr)
		, m_children(nullptr)
		, m_next(nullptr)
	{}

	RegisterInfo::RegisterInfo(RegisterType type, const char* name, const uint32 index, RegisterInfo* parent, const uint32 bitSize, const uint32 bitOffset)
		: m_type(type)
		, m_name(name)
		, m_index(index)
		, m_bitOffset(bitOffset)
		, m_bitSize(bitSize)
		, m_dataOffset(-1)
		, m_parent(parent)
		, m_children(nullptr)
		, m_next(nullptr)
	{
		RegisterInfo** lastPtr = &parent->m_children;
		while (*lastPtr)
			lastPtr = &((*lastPtr)->m_next);

		(*lastPtr) = this;
		m_next = nullptr;
	}

	RegisterInfo::~RegisterInfo()
	{
	}

	//----

	RegisterBankInfo::RegisterBankInfo()
	{
	}

	RegisterBankInfo::~RegisterBankInfo()
	{
		for (auto* ptr : m_allRegisters)
			delete ptr;
	}

	const RegisterInfo* RegisterBankInfo::FindRegister(const char* name) const
	{
		auto it = m_map.find(name);
		if (it != m_map.end())
			return it->second;

		return nullptr;
	}

	void RegisterBankInfo::Setup(const char* platformName, const char* cpuName)
	{
		m_platformName = platformName;
		m_cpuName = cpuName;
	}

	void RegisterBankInfo::AddRootRegister(RegisterInfo::RegisterType type, const char* name, const uint32 bitSize, const uint32 dataOffset)
	{
		const auto index = (uint32)m_allRegisters.size();
		auto* reg = new RegisterInfo(type, name, index, bitSize, dataOffset);
		m_allRegisters.push_back(reg);
		m_rootRegisters.push_back(reg);
		DEBUG_CHECK(FindRegister(name) == nullptr);
		m_map[name] = reg;
	}

	void RegisterBankInfo::AddChildRegister(RegisterInfo::RegisterType type, const char* parentName, const char* name, const uint32 bitSize, const uint32 bitOffset)
	{
		auto* parent = const_cast<RegisterInfo*>(FindRegister(parentName));
		DEBUG_CHECK(parent != nullptr);

		const auto index = (uint32)m_allRegisters.size();
		auto* reg = new RegisterInfo(type, name, index, parent, bitSize, bitOffset);
		m_allRegisters.push_back(reg);
		DEBUG_CHECK(FindRegister(name) == nullptr);
		m_map[name] = reg;
	}

	//---

	DeviceType IDeviceCPU::GetType() const
	{
		return DeviceType::CPU;
	}

	//---

} // runtime