
#pragma once

#include "launcherBase.h"
#include "runtimeDevice.h"

namespace runtime
{
	class RegisterBank;
	class RegisterInfo;

	/// cpu register description
	class LAUNCHER_API RegisterInfo
	{
	public:
		/// type of register
		enum RegisterType
		{
			Fixed,	// integer
			Float,  // floating point
			Flags,  // flags register
			Flag,   // single flag
			Wide,	// multicomponent register (SSE, MMX)
		};

		/// get register type
		inline const RegisterType GetType() const { return m_type; }

		/// get internal index
		inline const uint32 GetIndex() const { return m_index; }

		/// get data offset in the register bank, may return -1 for child registers
		inline const int32 GetDataOffset() const { return m_dataOffset; }

		/// get size of register's data in bits, 1, 8, 16, 32, 64, 128, 256, 512
		inline const uint32 GetBitCount() const { return m_bitSize; }

		/// get bit offset in parent register (ie. AH as offset of 8 bits within the AX)
		inline const uint32 GetBitOffset() const { return m_bitOffset; }

		/// get human readable name
		inline const char* GetName() const { return m_name;  }

		/// get parent register (i.e. EAX is parent of AX)
		inline const RegisterInfo* GetParent() const { return m_parent; }

		/// get child registers (ie. AX has two children: AH and AL)
		virtual const RegisterInfo* GetChildren() const { return m_children; }

		/// get next child register
		virtual const RegisterInfo* GetNext() const { return m_next;  }

		RegisterInfo(RegisterType type, const char* name, const uint32 index, const uint32 bitSize, const uint32 dataOffset);
		RegisterInfo(RegisterType type, const char* name, const uint32 index, RegisterInfo* parent, const uint32 bitSize, const uint32 bitOffset);
		~RegisterInfo();

	private:
		RegisterType	m_type;
		const char*		m_name;
		uint32			m_index;
		uint32			m_bitOffset;
		uint32			m_bitSize;
		int32			m_dataOffset;
		RegisterInfo*	m_parent;
		RegisterInfo*	m_children;
		RegisterInfo*	m_next;
	};

	/// cpu register bank
	class LAUNCHER_API RegisterBankInfo
	{
	public:
		RegisterBankInfo();
		~RegisterBankInfo();

		/// get name of the decoding platform
		inline const std::string& GetPlatformName() const { return m_platformName; }

		/// get name of the decoding CPU
		inline const std::string& GetCPUName() const { return m_cpuName; }

		/// get number of root registers on given platform
		inline const uint32 GetNumRootRegisters() const { return (uint32)m_rootRegisters.size(); }

		/// get root register
		inline const RegisterInfo* GetRootRegister(const uint32 index) const { return m_rootRegisters[index]; }

		/// find register by name, does not have to be root
		const RegisterInfo* FindRegister(const char* name) const;

		//---

		/// setup information
		void Setup(const char* platformName, const char* cpuName);

		/// add a root register
		void AddRootRegister(RegisterInfo::RegisterType type, const char* name, const uint32 bitSize, const uint32 dataOfrfset);

		/// add a child register
		void AddChildRegister(RegisterInfo::RegisterType type, const char* parentName, const char* name, const uint32 bitSize, const uint32 bitOffset);

	private:
		typedef std::vector< RegisterInfo* > TAllRegisters;
		TAllRegisters	m_allRegisters;

		typedef std::vector< const RegisterInfo* > TRootRegisters;
		TRootRegisters	m_rootRegisters;

		typedef std::map< std::string, const RegisterInfo* > TRegisterMap;
		TRegisterMap	m_map;

		std::string m_platformName;
		std::string m_cpuName;
	};

	// cpu description
	class LAUNCHER_API IDeviceCPU : public IDevice
	{
	public:
		/// get device type
		virtual DeviceType GetType() const override final;

		/// allocate register bank for storing register data
		virtual const RegisterBankInfo& GetRegisterBankInfo() const = 0;
	};

} // runtime