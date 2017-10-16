#pragma once

#include <math.h>
#include <stdint.h>
#include <intsafe.h>

#include "../host_core/runtimeRegisterBank.h"
#include "../host_core/runtimeImageInfo.h"
#include "../host_core/runtimeExceptions.h"

namespace cpu
{

	// use the processor intrinsics if possible
#define USE_INTRINSICS 1
#define USE_HW_CLZ     0

	// check asm call
#define ASM_CHECK(x) static_assert(x, "Unsupported instruction format")

	// Wide register index mangling
#define VREG_WORD_INDEX(x) ((x) ^ (1))
#define VREG_BYTE_INDEX(x) ((x) ^ (3))

	typedef uint64 TReg;
	typedef uint64 TAddr;
	typedef double TFReg;

	namespace mem
	{
		template< uint32 size >
		struct swapper
		{};

		template<>
		struct swapper<1>
		{
			template< typename T >
			static inline void load(T* ret, const void* mem)
			{
				*ret = *(const T*)mem;
			}

			template< typename T >
			static inline void store(void* mem, const T* data)
			{
				*(T*)mem = *data;
			}
		};

		template<>
		struct swapper<2>
		{
			template< typename T >
			static inline void load(T* ret, const void* mem)
			{
				*(uint16*)ret = _byteswap_ushort(*(uint16*)mem);
			}

			template< typename T >
			static inline void store(void* mem, const T* data)
			{
				*(uint16*)mem = _byteswap_ushort(*(const uint16*)data);
			}
		};

		template<>
		struct swapper<4>
		{
			template< typename T >
			static inline void load(T* ret, const void* mem)
			{
				*(uint32*)ret = _byteswap_ulong(*(uint32*)mem);
			}

			template< typename T >
			static inline void store(void* mem, const T* data)
			{
				*(uint32*)mem = _byteswap_ulong(*(const uint32*)data);
			}

			template< typename T >
			static inline void storeAtomic(void* mem, const T* data)
			{
				InterlockedExchange((volatile unsigned long*)mem, (const unsigned long)_byteswap_ulong(*(const unsigned long*)data));
			}
		};

		template<>
		struct swapper<8>
		{
			template< typename T >
			static inline void load(T* ret, const void* mem)
			{
				*(uint64*)ret = _byteswap_uint64(*(uint64*)mem);
			}

			template< typename T >
			static inline void store(void* mem, const T* data)
			{
				*(uint64*)mem = _byteswap_uint64(*(const uint64*)data);
			}

			template< typename T >
			static inline void storeAtomic(void* mem, const T* data)
			{
				InterlockedExchange64((volatile unsigned long long*)mem, (const unsigned long long) _byteswap_ulong(*(const unsigned long long*) data));
			}
		};

		template< typename T >
		static inline T load(const void* ptr)
		{
			T ret;
			swapper<sizeof(T)>::load(&ret, ptr);
			return ret;
		}

		template< typename T >
		static inline T loadAddr(const uint32 addr)
		{
			T ret;
			swapper<sizeof(T)>::load(&ret, (const void*)addr);
			return ret;
		}

		template< typename T >
		static inline void store(void* ptr, const T& val)
		{
			swapper<sizeof(T)>::store(ptr, &val);
		}

		template< typename T >
		static inline void storeAtomic(void* ptr, const T& val)
		{
			swapper<sizeof(T)>::storeAtomic(ptr, &val);
		}

		template< typename T >
		static inline void storeAddr(const uint32 addr, const T& val)
		{
			swapper<sizeof(T)>::store((void*)addr, &val);
		}

	} // mem

	union Bit16
	{
		uint16 u16;
		int16 i16;
	};

	union Bit32
	{
		uint32 u32;
		int32 i32;
	};

	union Bit64
	{
		uint64 u64;
		int64 i64;
	};

	static inline int64 ToSigned(const uint64 u)
	{
		Bit64 ret;
		ret.u64 = u;
		return ret.i64;
	}

	static inline int32 ToSigned(const uint32 u)
	{
		Bit32 ret;
		ret.u32 = u;
		return ret.i32;
	}

	static inline int16 ToSigned(const uint16 u)
	{
		Bit16 ret;
		ret.u16 = u;
		return ret.i16;
	}

	static inline uint64 ToUnsigned(const int64 i)
	{
		Bit64 ret;
		ret.i64 = i;
		return ret.u64;
	}

	static inline uint32 ToUnsigned(const int32 i)
	{
		Bit32 ret;
		ret.i32 = i;
		return ret.u32;
	}

	static inline uint16 ToUnsigned(const int16 i)
	{
		Bit16 ret;
		ret.i16 = i;
		return ret.u16;
	}

	static inline int64 ExtendSigned(const uint16 val)
	{
		Bit16 ret;
		ret.u16 = val;
		return ret.i16;
	}

	static inline int64 ExtendSigned(const uint32 val)
	{
		Bit32 ret;
		ret.u32 = val;
		return ret.i32;
	}

	struct ControlReg
	{
		uint8	lt;
		uint8	gt;
		uint8	eq;
		uint8	so;

		inline uint32 BuildMask() const
		{
			uint32 flags = 0;
			flags |= (lt ? 8 : 0);
			flags |= (gt ? 4 : 0);
			flags |= (eq ? 2 : 0);
			flags |= (so ? 1 : 0);
			return flags;
		}

		inline void Set(const uint32 mask)
		{
			lt = (mask & 8);
			gt = (mask & 4);
			eq = (mask & 2);
			so = (mask & 1);
		}

		inline bool operator==(const ControlReg& other) const
		{
			return *(const uint32*)this == *(const uint32*)&other;
		}
	};

	struct FixedReg
	{
		union
		{
			uint64 u64;
			int64 i64;

			struct
			{
				union
				{
					uint32 l;
					int32 li;
				};

				union
				{
					uint32 h;
					int32 hi;
				};
			};
		};
	};

	struct XerReg
	{
		uint8	so;
		uint8	ov;		// overflow
		uint8	ca;		// carry
		uint8	pa;		// padding

		inline uint32 BuildMask() const
		{
			uint32 flags = 0;
			flags |= so ? 1 : 0;
			flags |= ov ? 2 : 0;
			flags |= ca ? 4 : 0;
			flags |= pa ? 8 : 0;
			return flags;
		}

		inline bool operator==(const XerReg& other) const
		{
			return *(const uint32*)this == *(const uint32*)&other;
		}

		inline XerReg& operator=(const uint64 other)
		{
			so = (other & 1) != 0;
			ov = (other & 2) != 0;
			ca = (other & 4) != 0;
			return *this;
		}
	};

	struct VReg
	{
		union
		{
			struct
			{
				float f[4];
			};

			struct
			{
				uint32 u32[4];
			};

			struct
			{
				int i32[4];
			};

			struct
			{
				int16 i16[8];
			};

			struct
			{
				uint16 u16[8];
			};

			struct
			{
				uint8 u8[16];
			};
		};

		//---

		template< uint8 Index >
		float& AsFloat()
		{
			return ((float*)this)[Index];
		}

		template< uint8 Index >
		uint32& AsUint32()
		{
			return ((uint32*)this)[Index];
		}

		template< uint8 Index >
		int32& AsInt32()
		{
			return ((int32*)this)[Index];
		}

		template< uint8 Index >
		uint16& AsUint16()
		{
			return ((uint16*)this)[VREG_WORD_INDEX(Index)];
		}

		template< uint8 Index >
		int16& AsInt16()
		{
			return ((int16*)this)[VREG_WORD_INDEX(Index)];
		}

		template< uint8 Index >
		uint8& AsUint8()
		{
			return ((uint8*)this)[VREG_BYTE_INDEX(Index)];
		}

		template< uint8 Index >
		int8& AsInt8()
		{
			return ((int8*)this)[VREG_BYTE_INDEX(Index)];
		}

		//---

		float& AsFloat(const uint8 Index)
		{
			return ((float*)this)[Index];
		}

		uint32& AsUint32(const uint8 Index)
		{
			return ((uint32*)this)[Index];
		}

		int32& AsInt32(const uint8 Index)
		{
			return ((int32*)this)[Index];
		}

		uint16& AsUint16(const uint8 Index)
		{
			return ((uint16*)this)[VREG_WORD_INDEX(Index)];
		}

		int16& AsInt16(const uint8 Index)
		{
			return ((int16*)this)[VREG_WORD_INDEX(Index)];
		}

		uint8& AsUint8(const uint8 Index)
		{
			return ((uint8*)this)[VREG_BYTE_INDEX(Index)];
		}

		int8& AsInt8(const uint8 Index)
		{
			return ((int8*)this)[VREG_BYTE_INDEX(Index)];
		}

		//---

		float AsFloat(const uint8 Index) const
		{
			return ((float*)this)[Index];
		}

		uint32 AsUint32(const uint8 Index) const
		{
			return ((uint32*)this)[Index];
		}

		int32 AsInt32(const uint8 Index) const
		{
			return ((int32*)this)[Index];
		}

		uint16 AsUint16(const uint8 Index) const
		{
			return ((uint16*)this)[VREG_WORD_INDEX(Index)];
		}

		int16 AsInt16(const uint8 Index) const
		{
			return ((int16*)this)[VREG_WORD_INDEX(Index)];
		}

		uint8& AsUint8(const uint8 Index) const
		{
			return ((uint8*)this)[VREG_BYTE_INDEX(Index)];
		}

		int8 AsInt8(const uint8 Index) const
		{
			return ((int8*)this)[VREG_BYTE_INDEX(Index)];
		}

		//---

		template< uint8 Index >
		const float AsFloat() const
		{
			return ((const float*)this)[Index];
		}

		template< uint8 Index >
		const uint32 AsUint32() const
		{
			return ((const uint32*)this)[Index];
		}

		template< uint8 Index >
		const int32 AsInt32() const
		{
			return ((const int32*)this)[Index];
		}

		template< uint8 Index >
		const uint16 AsUint16() const
		{
			return ((const uint16*)this)[VREG_WORD_INDEX(Index)];
		}

		template< uint8 Index >
		const int16 AsInt16() const
		{
			return ((const int16*)this)[VREG_WORD_INDEX(Index)];
		}

		template< uint8 Index >
		const uint8 AsUint8() const
		{
			return ((const uint8*)this)[VREG_BYTE_INDEX(Index)];
		}

		template< uint8 Index >
		const int8 AsInt8() const
		{
			return ((const int8*)this)[VREG_BYTE_INDEX(Index)];
		}

		//---

		inline bool operator ==(const VReg& other) const
		{
			return (AsUint32<0>() == other.AsUint32<0>()) && (AsUint32<1>() == other.AsUint32<1>()) && (AsUint32<2>() == other.AsUint32<2>()) && (AsUint32<3>() == other.AsUint32<3>());
		}

		inline bool operator !=(const VReg& other) const
		{
			return (AsUint32<0>() != other.AsUint32<0>()) || (AsUint32<1>() != other.AsUint32<1>()) || (AsUint32<2>() != other.AsUint32<2>()) || (AsUint32<3>() != other.AsUint32<3>());
		}
	};

	typedef VReg TVReg;
	typedef void(*TLockFunc)();
	typedef void(*TUnlockFunc)();

	struct Reservation
	{
		volatile uint32		FLAG;
		volatile uint32		ADDR;
		TLockFunc			LOCK;
		TUnlockFunc			UNLOCK;
	};

	struct Interrupts
	{
		static const uint32 MAX_INTERRUPTS = 0x40;

		runtime::TInterruptFunc INTERRUPTS[MAX_INTERRUPTS];

		inline void CallInterrupt(const uint64_t ip, const uint64_t index, const runtime::RegisterBank& regs)
		{
			if (index >= MAX_INTERRUPTS)
			{
				runtime::InvalidInstruction(ip);
			}
			else
			{
				INTERRUPTS[index](ip, (uint32_t)index, regs);
			}
		}
	};

	struct GeneralIO
	{
		static const uint32 MAX_INTERRUPTS = 0x40;

		runtime::TGlobalMemReadFunc MEM_READ;
		runtime::TGlobalMemWriteFunc MEM_WRITE;
		runtime::TGlobalPortReadFunc PORT_READ;
		runtime::TGlobalPortWriteFunc PORT_WRITE;
	};

	class CpuRegs : public runtime::RegisterBank
	{
	public:
		CpuRegs();

		TReg		LR;
		TReg		CTR;
		TReg		MSR;
		XerReg		XER;

		ControlReg	CR[8];

		uint32		FPSCR;
		uint32		SAT;

		TReg		R0;
		TReg		R1;
		TReg		R2;
		TReg		R3;
		TReg		R4;
		TReg		R5;
		TReg		R6;
		TReg		R7;
		TReg		R8;
		TReg		R9;
		TReg		R10;
		TReg		R11;
		TReg		R12;
		TReg		R13;
		TReg		R14;
		TReg		R15;
		TReg		R16;
		TReg		R17;
		TReg		R18;
		TReg		R19;
		TReg		R20;
		TReg		R21;
		TReg		R22;
		TReg		R23;
		TReg		R24;
		TReg		R25;
		TReg		R26;
		TReg		R27;
		TReg		R28;
		TReg		R29;
		TReg		R30;
		TReg		R31;
		TReg		R32;

		TFReg		FR0;
		TFReg		FR1;
		TFReg		FR2;
		TFReg		FR3;
		TFReg		FR4;
		TFReg		FR5;
		TFReg		FR6;
		TFReg		FR7;
		TFReg		FR8;
		TFReg		FR9;
		TFReg		FR10;
		TFReg		FR11;
		TFReg		FR12;
		TFReg		FR13;
		TFReg		FR14;
		TFReg		FR15;
		TFReg		FR16;
		TFReg		FR17;
		TFReg		FR18;
		TFReg		FR19;
		TFReg		FR20;
		TFReg		FR21;
		TFReg		FR22;
		TFReg		FR23;
		TFReg		FR24;
		TFReg		FR25;
		TFReg		FR26;
		TFReg		FR27;
		TFReg		FR28;
		TFReg		FR29;
		TFReg		FR30;
		TFReg		FR31;

		VReg		VR0;
		VReg		VR1;
		VReg		VR2;
		VReg		VR3;
		VReg		VR4;
		VReg		VR5;
		VReg		VR6;
		VReg		VR7;
		VReg		VR8;
		VReg		VR9;
		VReg		VR10;
		VReg		VR11;
		VReg		VR12;
		VReg		VR13;
		VReg		VR14;
		VReg		VR15;
		VReg		VR16;
		VReg		VR17;
		VReg		VR18;
		VReg		VR19;
		VReg		VR20;
		VReg		VR21;
		VReg		VR22;
		VReg		VR23;
		VReg		VR24;
		VReg		VR25;
		VReg		VR26;
		VReg		VR27;
		VReg		VR28;
		VReg		VR29;
		VReg		VR30;
		VReg		VR31;
		VReg		VR32;
		VReg		VR33;
		VReg		VR34;
		VReg		VR35;
		VReg		VR36;
		VReg		VR37;
		VReg		VR38;
		VReg		VR39;
		VReg		VR40;
		VReg		VR41;
		VReg		VR42;
		VReg		VR43;
		VReg		VR44;
		VReg		VR45;
		VReg		VR46;
		VReg		VR47;
		VReg		VR48;
		VReg		VR49;
		VReg		VR50;
		VReg		VR51;
		VReg		VR52;
		VReg		VR53;
		VReg		VR54;
		VReg		VR55;
		VReg		VR56;
		VReg		VR57;
		VReg		VR58;
		VReg		VR59;
		VReg		VR60;
		VReg		VR61;
		VReg		VR62;
		VReg		VR63;
		VReg		VR64;
		VReg		VR65;
		VReg		VR66;
		VReg		VR67;
		VReg		VR68;
		VReg		VR69;
		VReg		VR70;
		VReg		VR71;
		VReg		VR72;
		VReg		VR73;
		VReg		VR74;
		VReg		VR75;
		VReg		VR76;
		VReg		VR77;
		VReg		VR78;
		VReg		VR79;
		VReg		VR80;
		VReg		VR81;
		VReg		VR82;
		VReg		VR83;
		VReg		VR84;
		VReg		VR85;
		VReg		VR86;
		VReg		VR87;
		VReg		VR88;
		VReg		VR89;
		VReg		VR90;
		VReg		VR91;
		VReg		VR92;
		VReg		VR93;
		VReg		VR94;
		VReg		VR95;
		VReg		VR96;
		VReg		VR97;
		VReg		VR98;
		VReg		VR99;
		VReg		VR100;
		VReg		VR101;
		VReg		VR102;
		VReg		VR103;
		VReg		VR104;
		VReg		VR105;
		VReg		VR106;
		VReg		VR107;
		VReg		VR108;
		VReg		VR109;
		VReg		VR110;
		VReg		VR111;
		VReg		VR112;
		VReg		VR113;
		VReg		VR114;
		VReg		VR115;
		VReg		VR116;
		VReg		VR117;
		VReg		VR118;
		VReg		VR119;
		VReg		VR120;
		VReg		VR121;
		VReg		VR122;
		VReg		VR123;
		VReg		VR124;
		VReg		VR125;
		VReg		VR126;
		VReg		VR127;

		Reservation* RES;
		Interrupts* INT;
		GeneralIO* IO;

		inline uint32 GetXERFlags() const
		{
			return XER.BuildMask();
		}

		inline uint32 GetControlRegsFlags() const
		{
			uint32 flags = 0;
			flags |= (CR[0].BuildMask() << 28);
			flags |= (CR[1].BuildMask() << 24);
			flags |= (CR[2].BuildMask() << 20);
			flags |= (CR[3].BuildMask() << 16);
			flags |= (CR[4].BuildMask() << 12);
			flags |= (CR[5].BuildMask() << 8);
			flags |= (CR[6].BuildMask() << 4);
			flags |= (CR[7].BuildMask() << 0);
			return flags;
		}

		// Register bank
		virtual uint64 ReturnFromFunction() override final;
	};
	
	// memory operations
	namespace mem
	{
		//------

		template< typename T >
		static inline T* to_ptr(const TAddr targetAddr)
		{
			return (T*)(void*)(size_t)(uint32)(targetAddr);
		}

		static inline void store64(CpuRegs& regs, const TReg value, const TAddr targetAddr)
		{
			*to_ptr<uint64>(targetAddr) = _byteswap_uint64(value);
		}

		static inline void store64volatile(CpuRegs& regs, const TReg value, const TAddr targetAddr)
		{
			*to_ptr<volatile uint64>(targetAddr) = _byteswap_uint64(value);
		}

		template< typename T >
		static inline void store32(CpuRegs& regs, const T value, const TAddr targetAddr)
		{
			union
			{
				T		v;
				uint64	u;
			} x;

			x.v = value;

			*to_ptr<uint32>(targetAddr) = _byteswap_ulong((uint32)x.u);
		}

		template< typename T >
		static inline void store32volatile(CpuRegs& regs, const T value, const TAddr targetAddr)
		{
			union
			{
				T		v;
				uint64	u;
			} x;

			x.v = value;

			*to_ptr<volatile uint32>(targetAddr) = _byteswap_ulong((uint32)x.u);
		}

		static inline void store16(CpuRegs& regs, const TReg value, const TAddr targetAddr)
		{
			*to_ptr<uint16>(targetAddr) = _byteswap_ushort((uint16)value);
		}

		static inline void store8(CpuRegs& regs, const TReg value, const TAddr targetAddr)
		{
			*to_ptr<uint8>(targetAddr) = (uint8)value;
		}

		static inline void store64f(CpuRegs& regs, const TFReg value, const TAddr targetAddr)
		{
			union
			{
				uint64 u64;
				double d;
			} x;
			x.d = (double)value;
			*to_ptr<uint64>(targetAddr) = _byteswap_uint64(x.u64);
		}

		static inline void store32f(CpuRegs& regs, const TFReg value, const TAddr targetAddr)
		{
			union
			{
				uint32 u32;
				float f;
			} x;
			x.f = (float)value;
			*to_ptr<uint32>(targetAddr) = _byteswap_ulong(x.u32);
		}


		//------

		static inline void load64z(CpuRegs& regs, TReg* value, const TAddr targetAddr)
		{
			*value = _byteswap_uint64(*to_ptr<uint64>(targetAddr));
		}

		static inline void load32z(CpuRegs& regs, TReg* value, const TAddr targetAddr)
		{
			*value = _byteswap_ulong(*to_ptr<uint32>(targetAddr));
		}

		static inline void load32zvolatile(CpuRegs& regs, TReg* value, const TAddr targetAddr)
		{
			*value = _byteswap_ulong(*to_ptr<volatile uint32>(targetAddr));
		}

		static inline void load16z(CpuRegs& regs, TReg* value, const TAddr targetAddr)
		{
			*value = _byteswap_ushort(*to_ptr<uint16>(targetAddr));
		}

		static inline void load8z(CpuRegs& regs, TReg* value, const TAddr targetAddr)
		{
			*value = *to_ptr<uint8>(targetAddr);
		}

		//------

		static inline void load64(CpuRegs& regs, TReg* value, const TAddr targetAddr)
		{
			*value = _byteswap_uint64(*to_ptr<uint64>(targetAddr));
		}

		static inline void load64volatile(CpuRegs& regs, TReg* value, const TAddr targetAddr)
		{
			*value = _byteswap_uint64(*to_ptr<volatile uint64>(targetAddr));
		}

		static inline void load32a(CpuRegs& regs, TReg* value, const TAddr targetAddr)
		{
			union
			{
				uint32 u32;
				int i32;
			} v;

			v.u32 = _byteswap_ulong(*to_ptr<uint32>(targetAddr));
			*(__int64*)value = v.i32;
		}

		static inline void load16a(CpuRegs& regs, TReg* value, const TAddr targetAddr)
		{
			union
			{
				uint16 u16;
				short i16;
			} v;

			v.u16 = _byteswap_ushort(*to_ptr<uint16>(targetAddr));
			*(__int64*)value = v.i16;
		}

		static inline void load8a(CpuRegs& regs, TReg* value, const TAddr targetAddr)
		{
			union
			{
				uint8 u8;
				char i8;
			} v;

			v.u8 = *to_ptr<uint8>(targetAddr);
			*(__int64*)value = v.i8;
		}

		static inline void load64f(CpuRegs& regs, TFReg* value, const TAddr targetAddr)
		{
			union
			{
				uint64 u64;
				double d;
			} x;

			x.u64 = _byteswap_uint64(*to_ptr<uint64>(targetAddr));
			*value = (TFReg)x.d;
		}

		static inline void load32f(CpuRegs& regs, TFReg* value, const TAddr targetAddr)
		{
			union
			{
				uint32 u32;
				float f;
			} x;

			x.u32 = _byteswap_ulong(*to_ptr<uint32>(targetAddr));
			*value = (TFReg)x.f;
		}

		//------

		// atomic load word with reserve
		static inline void lwarx(CpuRegs& regs, TReg* out, const uint32 addr)
		{
			regs.RES->LOCK();
			regs.RES->FLAG = 1;
			regs.RES->ADDR = addr;
			cpu::mem::load32zvolatile(regs, out, addr);
			//regs.RES->UNLOCK();
		}

		// atomic load double word with reserve
		static inline void ldarx(CpuRegs& regs, TReg* out, const uint32 addr)
		{
			regs.RES->LOCK();
			regs.RES->FLAG = 1;
			regs.RES->ADDR = addr;
			cpu::mem::load64volatile(regs, out, addr);
			//regs.RES->UNLOCK();
		}

		template<uint8 FLAG>
		static inline void seteq0(CpuRegs& regs)
		{
			regs.CR[0].lt = 0;
			regs.CR[0].gt = 0;
			regs.CR[0].eq = FLAG;
			regs.CR[0].so = regs.XER.so;
		}

		// atomic conditional store word with reserve
		static inline void stwcx(CpuRegs& regs, const TReg val, const uint32 addr)
		{
			//regs.RES->LOCK();
			if (regs.RES->FLAG && regs.RES->ADDR == addr)
			{
				cpu::mem::store32volatile(regs, (uint32)val, addr);
				cpu::mem::seteq0<1>(regs);
			}
			else
			{
				cpu::mem::seteq0<0>(regs);
			}

			regs.RES->FLAG = 0;
			regs.RES->UNLOCK();
		}

		// atomic conditional store double word with reserve
		static inline void stdcx(CpuRegs& regs, const TReg val, const uint32 addr)
		{
			//regs.RES->LOCK();
			if (regs.RES->FLAG && regs.RES->ADDR == addr)
			{
				cpu::mem::store64volatile(regs, val, addr);
				cpu::mem::seteq0<1>(regs);
			}
			else
			{
				cpu::mem::seteq0<0>(regs);
			}

			regs.RES->FLAG = 0;
			regs.RES->UNLOCK();
		}

		//------

		// load world inversed
		static inline void lwbrx(CpuRegs& regs, TReg* out, const uint32 addr)
		{
			*out = *to_ptr<uint32>(addr); // NO SWAP!
		}

		// load half word inversed
		static inline void lhbrx(CpuRegs& regs, TReg* out, const uint32 addr)
		{
			*out = *to_ptr<uint16>(addr); // NO SWAP!
		}

		// store word byte inversed
		template< typename T >
		static inline void stwbrx(CpuRegs& regs, const T& value, const uint32 addr)
		{
			union
			{
				T		v;
				uint64	u;
			} x;

			x.v = value;

			*to_ptr<uint32>(addr) = (uint32)x.u;
		}

		// store half word byte inversed
		template< typename T >
		static inline void sthbrx(CpuRegs& regs, const T& value, const uint32 addr)
		{
			union
			{
				T		v;
				uint64	u;
			} x;

			x.v = value;

			*to_ptr<uint16>(addr) = (uint32)x.u;
		}

		//------

		static inline void lvx(CpuRegs& regs, VReg* value, const TAddr addr)
		{
			value->AsUint32<0>() = _byteswap_ulong(((const uint32*)(addr & ~0xF))[0]);
			value->AsUint32<1>() = _byteswap_ulong(((const uint32*)(addr & ~0xF))[1]);
			value->AsUint32<2>() = _byteswap_ulong(((const uint32*)(addr & ~0xF))[2]);
			value->AsUint32<3>() = _byteswap_ulong(((const uint32*)(addr & ~0xF))[3]);
		}

		template< int SH >
		static inline void lvlx_helper(VReg* value, const VReg* srcValue)
		{
			uint8* dest = (uint8*)value;
			const uint8* src = (const uint8*)srcValue;
			int i = 0;

			for (i = 0; i < 16 - SH; ++i)
				dest[i] = src[i + SH];

			while (i < 16)
				dest[i++] = 0;

			value->AsUint32<0>() = _byteswap_ulong(value->AsUint32<0>());
			value->AsUint32<1>() = _byteswap_ulong(value->AsUint32<1>());
			value->AsUint32<2>() = _byteswap_ulong(value->AsUint32<2>());
			value->AsUint32<3>() = _byteswap_ulong(value->AsUint32<3>());
		}

		static inline void lvlx(CpuRegs& regs, VReg* value, const TAddr addr)
		{
			const TAddr pos = addr & 0x0F;
			const VReg* src = (const VReg*)(addr & ~0xF);

			switch (pos)
			{
				case 0: lvlx_helper<0>(value, src); break;
				case 1: lvlx_helper<1>(value, src); break;
				case 2: lvlx_helper<2>(value, src); break;
				case 3: lvlx_helper<3>(value, src); break;
				case 4: lvlx_helper<4>(value, src); break;
				case 5: lvlx_helper<5>(value, src); break;
				case 6: lvlx_helper<6>(value, src); break;
				case 7: lvlx_helper<7>(value, src); break;
				case 8: lvlx_helper<8>(value, src); break;
				case 9: lvlx_helper<9>(value, src); break;
				case 10: lvlx_helper<10>(value, src); break;
				case 11: lvlx_helper<11>(value, src); break;
				case 12: lvlx_helper<12>(value, src); break;
				case 13: lvlx_helper<13>(value, src); break;
				case 14: lvlx_helper<14>(value, src); break;
				case 15: lvlx_helper<15>(value, src); break;
			}
		}

		static inline void lvehx(CpuRegs& regs, VReg* value, const TAddr addr)
		{
			const TAddr pos = addr & ~1;
			const VReg* src = (const VReg*)(addr & ~0xF);
			value->AsUint16((pos >> 1) & 7) = _byteswap_ushort(*(const uint16*)pos);
		}

		template< int SH >
		static inline void lvrx_helper(VReg* value, const VReg* srcValue)
		{
			uint8* dest = (uint8*)value;
			const uint8* src = (const uint8*)srcValue;

			int i = 0;

			while (i < (16 - SH))
			{
				dest[i] = 0;
				++i;
			}

			while (i < 16)
			{
				dest[i] = src[i - (16 - SH)];
				++i;
			}

			value->AsUint32<0>() = _byteswap_ulong(value->AsUint32<0>());
			value->AsUint32<1>() = _byteswap_ulong(value->AsUint32<1>());
			value->AsUint32<2>() = _byteswap_ulong(value->AsUint32<2>());
			value->AsUint32<3>() = _byteswap_ulong(value->AsUint32<3>());
		}

		static inline void lvrx(CpuRegs& regs, VReg* value, const TAddr addr)
		{
			const TAddr pos = addr & 0x0F;
			const VReg* src = (const VReg*)(addr & ~0xF);

			switch (pos)
			{
				case 0: lvrx_helper<0>(value, src); break;
				case 1: lvrx_helper<1>(value, src); break;
				case 2: lvrx_helper<2>(value, src); break;
				case 3: lvrx_helper<3>(value, src); break;
				case 4: lvrx_helper<4>(value, src); break;
				case 5: lvrx_helper<5>(value, src); break;
				case 6: lvrx_helper<6>(value, src); break;
				case 7: lvrx_helper<7>(value, src); break;
				case 8: lvrx_helper<8>(value, src); break;
				case 9: lvrx_helper<9>(value, src); break;
				case 10: lvrx_helper<10>(value, src); break;
				case 11: lvrx_helper<11>(value, src); break;
				case 12: lvrx_helper<12>(value, src); break;
				case 13: lvrx_helper<13>(value, src); break;
				case 14: lvrx_helper<14>(value, src); break;
				case 15: lvrx_helper<15>(value, src); break;
			}
		}

		static inline void stvx(CpuRegs& regs, VReg value, const TAddr addr)
		{
			((uint32*)(addr & ~0xF))[0] = _byteswap_ulong(value.AsUint32<0>());
			((uint32*)(addr & ~0xF))[1] = _byteswap_ulong(value.AsUint32<1>());
			((uint32*)(addr & ~0xF))[2] = _byteswap_ulong(value.AsUint32<2>());
			((uint32*)(addr & ~0xF))[3] = _byteswap_ulong(value.AsUint32<3>());
		}

		static inline void stvehx(CpuRegs& regs, VReg value, const TAddr addr)
		{
			struct
			{
				uint32 v[4];
			} temp;

			temp.v[0] = _byteswap_ulong(value.AsUint32<0>());
			temp.v[1] = _byteswap_ulong(value.AsUint32<1>());
			temp.v[2] = _byteswap_ulong(value.AsUint32<2>());
			temp.v[3] = _byteswap_ulong(value.AsUint32<3>());

			if (addr & 3)
				runtime::UnimplementedImportFunction(0, "stvewx - alignment error");

			const uint32 part = (addr & 0xF) & ~2;
			//printf( "EA = 0x%08X, part=%d\n", addr, part );

			const uint16 data = *(uint16*)((uint8*)&temp + part);
			//printf( "Data=%d (0x%04X)\n", data, data );
			//printf( "Reg=%08X,%08X,%08X,%08X\n", value.AsUint32<0>(), value.AsUint32<1>(), value.AsUint32<2>(), value.AsUint32<3>());

			*(uint16*)addr = data;
		}

		static inline void stvewx(CpuRegs& regs, VReg value, const TAddr addr)
		{
			struct
			{
				uint32 v[4];
			} temp;

			temp.v[0] = _byteswap_ulong(value.AsUint32<0>());
			temp.v[1] = _byteswap_ulong(value.AsUint32<1>());
			temp.v[2] = _byteswap_ulong(value.AsUint32<2>());
			temp.v[3] = _byteswap_ulong(value.AsUint32<3>());

			if (addr & 3)
				runtime::UnimplementedImportFunction(0, "stvewx - alignment error");

			const uint32 part = (addr & 0xF) & ~3;
			//printf( "EA = 0x%08X, part=%d\n", addr, part );

			const uint32 data = *(uint32*)((uint8*)&temp + part);
			//printf( "Data=%d (0x%08X)\n", data, data );
			//printf( "Reg=%08X,%08X,%08X,%08X\n", value.AsUint32<0>(), value.AsUint32<1>(), value.AsUint32<2>(), value.AsUint32<3>());

			*(uint32*)addr = data;
		}

		template< int SH >
		static inline void stvlx_helper(VReg* value, const VReg& srcValueX)
		{
			VReg srcValue;
			srcValue.AsUint32<0>() = _byteswap_ulong(srcValueX.AsUint32<0>());
			srcValue.AsUint32<1>() = _byteswap_ulong(srcValueX.AsUint32<1>());
			srcValue.AsUint32<2>() = _byteswap_ulong(srcValueX.AsUint32<2>());
			srcValue.AsUint32<3>() = _byteswap_ulong(srcValueX.AsUint32<3>());

			uint8* dest = (uint8*)value;
			const uint8* src = (const uint8*)&srcValue;

			for (int i = 0; i < 16 - SH; ++i)
				dest[i + SH] = src[i];
		}

		static inline void stvlx(CpuRegs& regs, VReg src, const TAddr addr)
		{
			const TAddr pos = addr & 0x0F;
			VReg* value = (VReg*)(addr & ~0xF);

			switch (pos)
			{
				case 0: stvlx_helper<0>(value, src); break;
				case 1: stvlx_helper<1>(value, src); break;
				case 2: stvlx_helper<2>(value, src); break;
				case 3: stvlx_helper<3>(value, src); break;
				case 4: stvlx_helper<4>(value, src); break;
				case 5: stvlx_helper<5>(value, src); break;
				case 6: stvlx_helper<6>(value, src); break;
				case 7: stvlx_helper<7>(value, src); break;
				case 8: stvlx_helper<8>(value, src); break;
				case 9: stvlx_helper<9>(value, src); break;
				case 10: stvlx_helper<10>(value, src); break;
				case 11: stvlx_helper<11>(value, src); break;
				case 12: stvlx_helper<12>(value, src); break;
				case 13: stvlx_helper<13>(value, src); break;
				case 14: stvlx_helper<14>(value, src); break;
				case 15: stvlx_helper<15>(value, src); break;
			}
		}

		template< int SH >
		static inline void stvrx_helper(VReg* value, const VReg& srcValueX)
		{
			VReg srcValue;
			srcValue.AsUint32<0>() = _byteswap_ulong(srcValueX.AsUint32<0>());
			srcValue.AsUint32<1>() = _byteswap_ulong(srcValueX.AsUint32<1>());
			srcValue.AsUint32<2>() = _byteswap_ulong(srcValueX.AsUint32<2>());
			srcValue.AsUint32<3>() = _byteswap_ulong(srcValueX.AsUint32<3>());

			uint8* dest = (uint8*)value;
			const uint8* src = (const uint8*)&srcValue;

			for (int i = 0; i < SH; ++i)
				dest[i] = src[(16 - SH) + i];
		}

		static inline void stvrx(CpuRegs& regs, VReg src, const TAddr addr)
		{
			const TAddr pos = addr & 0x0F;
			VReg* value = (VReg*)(addr & ~0xF);

			switch (pos)
			{
				case 0: stvrx_helper<0>(value, src); break;
				case 1: stvrx_helper<1>(value, src); break;
				case 2: stvrx_helper<2>(value, src); break;
				case 3: stvrx_helper<3>(value, src); break;
				case 4: stvrx_helper<4>(value, src); break;
				case 5: stvrx_helper<5>(value, src); break;
				case 6: stvrx_helper<6>(value, src); break;
				case 7: stvrx_helper<7>(value, src); break;
				case 8: stvrx_helper<8>(value, src); break;
				case 9: stvrx_helper<9>(value, src); break;
				case 10: stvrx_helper<10>(value, src); break;
				case 11: stvrx_helper<11>(value, src); break;
				case 12: stvrx_helper<12>(value, src); break;
				case 13: stvrx_helper<13>(value, src); break;
				case 14: stvrx_helper<14>(value, src); break;
				case 15: stvrx_helper<15>(value, src); break;
			}
		}

		//------

	}

	// math
	namespace math
	{
		class Float16Compressor
		{
			union Bits
			{
				float f;
				int si;
				uint32 ui;
			};

			static int const shift = 13;
			static int const shiftSign = 16;

			static int const infN = 0x7F800000; // flt32 infinity
			static int const maxN = 0x477FE000; // max flt16 normal as a flt32
			static int const minN = 0x38800000; // min flt16 normal as a flt32
			static int const signN = 0x80000000; // flt32 sign bit

			static int const infC = infN >> shift;
			static int const nanN = (infC + 1) << shift; // minimum flt16 nan as a flt32
			static int const maxC = maxN >> shift;
			static int const minC = minN >> shift;
			static int const signC = signN >> shiftSign; // flt16 sign bit

			static int const mulN = 0x52000000; // (1 << 23) / minN
			static int const mulC = 0x33800000; // minN / (1 << (23 - shift))

			static int const subC = 0x003FF; // max flt32 subnormal down shifted
			static int const norC = 0x00400; // min flt32 normal down shifted

			static int const maxD = infC - maxC - 1;
			static int const minD = minC - subC - 1;

		public:
			static uint16 Compress(uint32 value)
			{
				Bits v, s;
				v.ui = value;
				uint32 sign = v.si & signN;
				v.si ^= sign;
				sign >>= shiftSign; // logical shift
				s.si = mulN;
				s.si = (int)(s.f * v.f); // correct subnormals
				v.si ^= (s.si ^ v.si) & -(minN > v.si);
				v.si ^= (infN ^ v.si) & -((infN > v.si) & (v.si > maxN));
				v.si ^= (nanN ^ v.si) & -((nanN > v.si) & (v.si > infN));
				v.ui >>= shift; // logical shift
				v.si ^= ((v.si - maxD) ^ v.si) & -(v.si > maxC);
				v.si ^= ((v.si - minD) ^ v.si) & -(v.si > subC);
				return v.ui | sign;
			}

			static uint32 Decompress(uint16 value)
			{
				Bits v;
				v.ui = value;
				int sign = v.si & signC;
				v.si ^= sign;
				sign <<= shiftSign;
				v.si ^= ((v.si + minD) ^ v.si) & -(v.si > subC);
				v.si ^= ((v.si + maxD) ^ v.si) & -(v.si > maxC);
				Bits s;
				s.si = mulC;
				s.f *= v.si;
				int mask = -(norC > v.si);
				v.si <<= shift;
				v.si ^= (s.si ^ v.si) & mask;
				v.si |= sign;
				return v.ui;
			}
		};

		static inline uint32 FromHalf(const uint16 in)
		{
			return Float16Compressor::Decompress(in);
		}

		static inline uint16 ToHalf(const uint32 inu)
		{
			return Float16Compressor::Compress(inu);
		}
	}

	// compare
	namespace cmp
	{
		// compare signed values
		template <uint8 CR_INDEX>
		static inline void CmpSigned(CpuRegs& regs, __int64 a, __int64 b)
		{
			if (a < b)
			{
				regs.CR[CR_INDEX].lt = 1;
				regs.CR[CR_INDEX].gt = 0;
				regs.CR[CR_INDEX].eq = 0;
			}
			else if (a > b)
			{
				regs.CR[CR_INDEX].lt = 0;
				regs.CR[CR_INDEX].gt = 1;
				regs.CR[CR_INDEX].eq = 0;
			}
			else
			{
				regs.CR[CR_INDEX].lt = 0;
				regs.CR[CR_INDEX].gt = 0;
				regs.CR[CR_INDEX].eq = 1;
			}
		}

		// compare signed values, copy XER
		template <uint8 CR_INDEX>
		static inline void CmpSignedXER(CpuRegs& regs, __int64 a, __int64 b)
		{
			if (a < b)
			{
				regs.CR[CR_INDEX].lt = 1;
				regs.CR[CR_INDEX].gt = 0;
				regs.CR[CR_INDEX].eq = 0;
			}
			else if (a > b)
			{
				regs.CR[CR_INDEX].lt = 0;
				regs.CR[CR_INDEX].gt = 1;
				regs.CR[CR_INDEX].eq = 0;
			}
			else
			{
				regs.CR[CR_INDEX].lt = 0;
				regs.CR[CR_INDEX].gt = 0;
				regs.CR[CR_INDEX].eq = 1;
			}

			regs.CR[CR_INDEX].so = regs.XER.so;
		}

		// compare unsigned values
		template <uint8 CR_INDEX>
		static inline void CmpUnsigned(CpuRegs& regs, unsigned __int64 a, unsigned __int64 b)
		{
			if (a < b)
			{
				regs.CR[CR_INDEX].lt = 1;
				regs.CR[CR_INDEX].gt = 0;
				regs.CR[CR_INDEX].eq = 0;
			}
			else if (a > b)
			{
				regs.CR[CR_INDEX].lt = 0;
				regs.CR[CR_INDEX].gt = 1;
				regs.CR[CR_INDEX].eq = 0;
			}
			else
			{
				regs.CR[CR_INDEX].lt = 0;
				regs.CR[CR_INDEX].gt = 0;
				regs.CR[CR_INDEX].eq = 1;
			}
		}

		// compare unsigned values, copy XER value
		template <uint8 CR_INDEX>
		static inline void CmpUnsignedXER(CpuRegs& regs, unsigned __int64 a, unsigned __int64 b)
		{
			if (a < b)
			{
				regs.CR[CR_INDEX].lt = 1;
				regs.CR[CR_INDEX].gt = 0;
				regs.CR[CR_INDEX].eq = 0;
			}
			else if (a > b)
			{
				regs.CR[CR_INDEX].lt = 0;
				regs.CR[CR_INDEX].gt = 1;
				regs.CR[CR_INDEX].eq = 0;
			}
			else
			{
				regs.CR[CR_INDEX].lt = 0;
				regs.CR[CR_INDEX].gt = 0;
				regs.CR[CR_INDEX].eq = 1;
			}

			regs.CR[CR_INDEX].so = regs.XER.so;
		}

		// compare result of logical function, copy XER value
		template <uint8 CR_INDEX>
		static inline void CmpLogicalXER(CpuRegs& regs, unsigned __int64 a)
		{
			CmpSignedXER<CR_INDEX>(regs, (int64&)a, 0);
		}
	}

	// opcodes
	namespace op
	{

		// sign extend for 32 bits
		static inline TReg EXTS(const uint32 s)
		{
			union
			{
				uint32 u;
				int32 i;
			} w;

			union
			{
				uint64 u;
				int64 i;
			} d;

			w.u = s;
			d.i = w.i;
			return d.u;
		}

		// sign extend for 16 bits
		static inline TReg EXTS16(const uint16 s)
		{
			union
			{
				uint16 u;
				int16 i;
			} w;

			union
			{
				uint64 u;
				int64 i;
			} d;

			w.u = s;
			d.i = w.i;
			return d.u;
		}


		// no operation
		static inline void nop()
		{
		}

		// li - load immediate
		template <uint8 CTRL>
		static inline void li(CpuRegs& regs, TReg* out, const uint32 imm)
		{
			ASM_CHECK(CTRL == 0);
			*out = EXTS(imm);
		}

		// lis - load immediate shifted
		template <uint8 CTRL>
		static inline void lis(CpuRegs& regs, TReg* out, const uint32 imm)
		{
			ASM_CHECK(CTRL == 0);
			*out = EXTS(imm << 16);
		}

		//---------------------------------------------------------------------------------

		// addi - add immediate
		template <uint8 CTRL>
		static inline void addi(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			*out = a + EXTS(imm);
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// addis - add immediate shifter
		template <uint8 CTRL>
		static inline void addis(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			*out = a + EXTS(imm << 16);
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// addic - add immediate with the update of the carry flag
		template <uint8 CTRL>
		static inline void addic(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			*out = a + EXTS(imm);
			regs.XER.ca = (*out < a); // carry assuming there was no carry before
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// subfic - subtract immediate with the update of the carry flag
		template <uint8 CTRL>
		static inline void subfic(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			*out = ~a + EXTS(imm) + 1;
			regs.XER.ca = (*out < ~a); // carry assuming there was no carry before
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// adde - Add Extended XO-form
		template <uint8 CTRL>
		static inline void adde(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = a + b + regs.XER.ca;
			regs.XER.ca = (*out < a); // carry assuming there was no carry before
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// addme - Add to Minus One Extended XO-form
		template <uint8 CTRL>
		static inline void addme(CpuRegs& regs, TReg* out, const TReg a)
		{
			*out = a + regs.XER.ca - 1;
			regs.XER.ca = (*out < a); // carry assuming there was no carry before
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// subfe - Subtract From Extended XO-form
		template <uint8 CTRL>
		static inline void subfe(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = ~a + b + regs.XER.ca;
			regs.XER.ca = (*out < ~a); // carry assuming there was no carry before
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// subfme - Subtract From Minus One Extended XO-form
		template <uint8 CTRL>
		static inline void subfme(CpuRegs& regs, TReg* out, const TReg a)
		{
			*out = ~a + regs.XER.ca - 1;
			regs.XER.ca = (*out < ~a); // carry assuming there was no carry before
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// subfc - Subtract From Carrying XO-form
		template <uint8 CTRL>
		static inline void subfc(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = ~a + b + 1;
			regs.XER.ca = (*out < ~a); // carry assuming there was no carry before
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// addze - Add to Zero Extended XO-form
		template <uint8 CTRL>
		static inline void addze(CpuRegs& regs, TReg* out, const TReg a)
		{
			*out = a + regs.XER.ca;
			regs.XER.ca = (*out < a); // carry assuming there was no carry before
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// subfze - Subtract From Zero Extended XO-form
		template <uint8 CTRL>
		static inline void subfze(CpuRegs& regs, TReg* out, const TReg a)
		{
			*out = ~a + regs.XER.ca;
			regs.XER.ca = (*out < ~a); // carry assuming there was no carry before
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		//---------------------------------------------------------------------------------

		// extsb - extend sign byte
		template <uint8 CTRL>
		static inline void extsb(CpuRegs& regs, TReg* out, const TReg a)
		{
			const uint8 u8 = (uint8)(a);
			const int8 i8 = (const int8&)u8;
			*(int64*)out = i8;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// extsh - extend sign half word
		template <uint8 CTRL>
		static inline void extsh(CpuRegs& regs, TReg* out, const TReg a)
		{
			const uint16 u16 = (uint16)(a);
			const int16 i16 = (const int16&)u16;
			*(int64*)out = i16;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// extsw - extend sign word
		template <uint8 CTRL>
		static inline void extsw(CpuRegs& regs, TReg* out, const TReg a)
		{
			const uint32 u32 = (uint32)(a);
			const int32 i32 = (const int32&)u32;
			*(int64*)out = i32;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		//---------------------------------------------------------------------------------

		// add - signed add
		template <uint8 CTRL>
		static inline void add(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = a + b;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// addc - signed add with carrying
		template <uint8 CTRL>
		static inline void addc(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = a + b;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// sub - signed subtract FROM (b-a)
		template <uint8 CTRL>
		static inline void subf(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			// b-a 
			// 0 0, FF 0 1 => 0
			// 0 1, FF 1 1 => 1
			// 0 2, FF 2 1 => 2
			// 1 0, FE 0 1 => FF (-1)
			// 1 1, FE 1 1 => 0
			// 1 2, FE 2 1 => 1
			*out = ~a + b + 1;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// neg - Negate XO-form
		template <uint8 CTRL>
		static inline void neg(CpuRegs& regs, TReg* out, const TReg a)
		{
			*out = ~a + 1;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// mulli - Multiply Low Immediate D-form
		template <uint8 CTRL>
		static inline void mulli(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			ASM_CHECK(CTRL == 0);
			const auto temp = (int64)a * EXTS(imm);
			*out = (uint64&)temp;
		}

		// mulld - Multiply Low Doubleword XO-form
		template <uint8 CTRL>
		static inline void mulld(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			auto temp = (int64&)a * (int64&)b;
			*out = temp;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// mullw - Multiply Low Word XO-form
		template <uint8 CTRL>
		static inline void mullw(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			auto temp = (int64)(int32)a * (int64)(int32)b;
			*out = (uint64&)temp;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// divdu - Divide Doubleword Unsigned XO-form
		template <uint8 CTRL>
		static inline void divdu(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = a / b;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// divwu - Divide Word Unsigned XO-form
		template <uint8 CTRL>
		static inline void divwu(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = (uint64)(uint32)a / (uint64)(uint32)b;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// divd - Divide Doubleword XO-form
		template <uint8 CTRL>
		static inline void divd(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*(int64*)out = (int64&)a / (int64&)b;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// divw - Divide Word XO-form
		template <uint8 CTRL>
		static inline void divw(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			uint32 ua = (uint32)a;
			uint32 ub = (uint32)b;
			*(int64*)out = (int32&)ua / (int32&)ub;
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// mulhwu - Multiply High Word Unsigned XO-form
		template <uint8 CTRL>
		static inline void mulhwu(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			uint64 ret = (uint64)a * (uint64)b;
			*out = (uint32)(ret >> 32);
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		// mullhw - Multiply Low Halfword to Word Signed
		template <uint8 CTRL>
		static inline void mullhw(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			uint64 ua = EXTS16((uint16)a);
			uint64 ub = EXTS16((uint16)b);
			int64 as = (int64&)ua;
			int64 bs = (int64&)ub;
			int64 ret = as * bs;
			*(int64*)out = (ret & 0xFFFFFFFF);
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}

		static uint64 mulhi(const uint64 a, const uint64 b)
		{
			uint64 a_lo = (uint32)a;
			uint64 a_hi = a >> 32;
			uint64 b_lo = (uint32)b;
			uint64 b_hi = b >> 32;

			uint64 a_x_b_hi = a_hi * b_hi;
			uint64 a_x_b_mid = a_hi * b_lo;
			uint64 b_x_a_mid = b_hi * a_lo;
			uint64 a_x_b_lo = a_lo * b_lo;

			uint64 carry_bit = ((uint64)(uint32)a_x_b_mid + (uint64)(uint32)b_x_a_mid + (a_x_b_lo >> 32)) >> 32;
			uint64 multhi = a_x_b_hi + (a_x_b_mid >> 32) + (b_x_a_mid >> 32) + carry_bit;
			return multhi;
		}

		// mullhw - Multiply Low Halfword to Word Signed
		template <uint8 CTRL>
		static inline void mulhdu(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = mulhi(a, b);
			if (CTRL) cmp::CmpSignedXER<0>(regs, *(int64*)out, 0);
		}


		//---------------------------------------------------------------------------------

		// ori - or with immediate
		template <uint8 CTRL>
		static inline void ori(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			ASM_CHECK(CTRL == 0);
			*out = a | (TReg)imm;
		}

		// oris - or with immediate shifted
		template <uint8 CTRL>
		static inline void oris(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			ASM_CHECK(CTRL == 0);
			*out = a | (TReg)(imm << 16);
		}

		// xori - xor with immediate
		template <uint8 CTRL>
		static inline void xori(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			ASM_CHECK(CTRL == 0);
			*out = a ^ (TReg)imm;
		}

		// xoris - xor with immediate shifted
		template <uint8 CTRL>
		static inline void xoris(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			ASM_CHECK(CTRL == 0);
			*out = a ^ (TReg)(imm << 16);
		}

		// andi - and with immediate
		template <uint8 CTRL>
		static inline void andi(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			ASM_CHECK(CTRL == 1); // watch out!
			*out = a & (TReg)imm;
			cmp::CmpLogicalXER<0>(regs, *out);
		}

		// andis - or with immediate shifted
		template <uint8 CTRL>
		static inline void andis(CpuRegs& regs, TReg* out, const TReg a, const uint32 imm)
		{
			ASM_CHECK(CTRL == 1); // watch out!
			*out = a & (TReg)(imm << 16);
			cmp::CmpLogicalXER<0>(regs, *out);
		}

		//---------------------------------------------------------------------------------

		// logical or 
		template <uint8 CTRL>
		static inline void or (CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = a | b;
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// logical and
		template <uint8 CTRL>
		static inline void and(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = a & b;
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// logical xor
		template <uint8 CTRL>
		static inline void xor(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = a ^ b;
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// logical nand
		template <uint8 CTRL>
		static inline void nand(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = ~(a & b);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// logical nor 
		template <uint8 CTRL>
		static inline void nor(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = ~(a | b);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// logical eqv
		template <uint8 CTRL>
		static inline void eqv(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = ~(a ^ b);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// logical and with complement
		template <uint8 CTRL>
		static inline void andc(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = a & ~b;
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// logical or with complement
		template <uint8 CTRL>
		static inline void orc(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			*out = a | ~b;
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		//---------------------------------------------------------------------------------

		// ROTL for 32-bit values
		template <uint8 N>
		static inline uint32 RotL32(const uint32 val)
		{
#if USE_INTRINSICS
			return _rotl(val, N);
#else
			ASM_CHECK(N >= 0 && N <= 31);
			return (val << N) | (val >> (31 - N));
#endif
		}

		// ROTL for 32-bit values
		static inline uint32 RotL32N(const uint32 val, const uint32 N)
		{
#if USE_INTRINSICS
			return _rotl(val, N);
#else
			ASM_CHECK(N >= 0 && N <= 31);
			return (val << N) | (val >> (31 - N));
#endif
		}

		// ROTL for 64-bit values
		template <uint8 N>
		static inline uint64 RotL64(const uint64 val)
		{
#if USE_INTRINSICS
			return _rotl64(val, N);
#else
			ASM_CHECK(N >= 0 && N <= 63);
			return (val << N) | (val >> (63 - N));
#endif
		}

		// Build 111110000 bit pattern (M = number of zeros, 0-32)
		template <uint8 M>
		static inline uint32 HalfMask32LE()
		{
			static const uint32 Z = ~(uint32)0;
			return Z << (M);
		}

		// Build 111110000 bit pattern (M = number of zeros, 0-32)
		template <>
		static inline uint32 HalfMask32LE<32>()
		{
			return 0;
		}

		// Build 111110000 bit pattern (M = number of zeros, 0-64)
		template <uint8 M>
		static inline uint64 HalfMask64LE()
		{
			static const uint64 Z = ~(uint64)0;
			return Z << (M);
		}

		// Build 111110000 bit pattern (M = number of zeros, 0-64)
		template <>
		static inline uint64 HalfMask64LE<64>()
		{
			return 0;
		}

		// Build 111110000 bit pattern (M = number of zeros, 0-64)
		template <uint8 M>
		static inline uint64 HalfMask64BE()
		{
			static const uint64 Z = ~(uint64)0;
			return Z >> (M);
		}

		// Build 111110000 bit pattern (M = number of zeros, 0-64)
		template <>
		static inline uint64 HalfMask64BE<64>()
		{
			return 0;
		}

		// Mask builder (bits MB to ME set to 1), big endian (xbox native)
		template <uint8 MB, uint8 ME>
		static inline uint32 Mask32LE()
		{
			if (MB < ME + 1)
			{
				return HalfMask32LE< ME + 1 >() ^ HalfMask32LE< MB >();
			}
			else if (MB > ME)
			{
				// M(MB) | ~M(ME+1)
				return HalfMask32LE< MB >() | ~HalfMask32LE< ME + 1 >();
			}
			else
			{
				return 0xFFFFFFFF;
			}
		}

		// Mask builder (bits MB to ME set to 1), big endian (xbox native)
		template <uint8 MB, uint8 ME>
		static inline uint64 Mask64LE()
		{
			if (MB < ME + 1)
			{
				return HalfMask64LE< ME + 1 >() ^ HalfMask64LE< MB >();
			}
			else if (MB > ME)
			{
				// M(MB) | ~M(ME+1)
				return HalfMask64LE< MB >() | ~HalfMask64LE< ME + 1 >();
			}
			else
			{
				return (uint64)-1;
			}
		}

		// Mask builder (bits MB to ME set to 1), big endian (xbox native)
		template <uint8 MB, uint8 ME>
		static inline uint32 Mask32BE()
		{
			return Mask32LE<31 - ME, 31 - MB>(); // note the swapped direction
												 //return Mask32LE<ME, MB>(); // note the swapped direction
		}

		// Mask builder (bits MB to ME set to 1), big endian (xbox native)
		template <uint8 MB, uint8 ME>
		static inline uint64 Mask64BE()
		{
			return Mask64LE<63 - ME, 63 - MB>(); // note the swapped direction
												 //return Mask32LE<ME, MB>(); // note the swapped direction
		}

		// Rotate Left Word Immediate then AND with Mask M-form
		template <uint8 CTRL, uint8 N, uint8 MB, uint8 ME>
		static inline void rlwinm(CpuRegs& regs, TReg* out, const TReg a)
		{
			const uint32 r = RotL32<N>((uint32)a);
			const uint32 m = Mask32BE<MB, ME>();
			*(uint64*)out = r & m;
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// Rotate Left Word Immediate then Mask Insert M-form
		template <uint8 CTRL, uint8 N, uint8 MB, uint8 ME>
		static inline void rlwimi(CpuRegs& regs, TReg* out, const TReg a)
		{
			const uint64 r = RotL32<N>((uint32)a);
			const uint64 m = Mask32BE<MB, ME>();
			*(uint64*)out = (r&m) | (*(const uint64*)out & ~m);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// Rotate Left DoubleWord Immediate then AND with Mask M-form
		template <uint8 CTRL, uint8 N, uint8 MB, uint8 ME>
		static inline void rldinm(CpuRegs& regs, TReg* out, const TReg a)
		{
			const uint64 r = RotL64<N>(a);
			const uint64 m = Mask64BE<MB, ME>();
			*(uint64*)out = r & m;
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// Rotate Left DoubleWord Immediate then Mask Insert M-form
		template <uint8 CTRL, uint8 N, uint8 MB>
		static inline void rldimi(CpuRegs& regs, TReg* out, const TReg a)
		{
			const uint64 r = RotL64<N>(a);
			const uint64 m = Mask64BE<MB, 63 - N>();
			*(uint64*)out = (r&m) | (*(const uint64*)out & ~m);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// Rotate Left Doubleword Immediate then Clear Left MD-form
		template <uint8 CTRL, uint8 N, uint8 MB>
		static inline void rldicl(CpuRegs& regs, TReg* out, const TReg a)
		{
			const uint64 r = RotL64<N>(a);
			const uint64 m = HalfMask64BE<MB>();
			*(uint64*)out = r&m;
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// Rotate Left Doubleword Immediate then Clear Right MD-form
		template <uint8 CTRL, uint8 N, uint8 ME>
		static inline void rldicr(CpuRegs& regs, TReg* out, const TReg a)
		{
			const uint64 r = RotL64<N>(a);
			const uint64 m = ~HalfMask64BE<ME + 1>();
			*(uint64*)out = r&m;
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// Rotate Left Word then AND with Mask M-form
		template < uint8 CTRL, uint8 MB, uint8 ME >
		static inline void rlwnm(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			const uint8 n = (b & 31);
			const uint32 r = RotL32N((uint32)a, n);
			const uint32 m = Mask32BE<MB, ME>();
			*(uint64*)out = r & m;
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		//---------------------------------------------------------------------------------

		// sld - Shift Left Doubleword X-form
		template <uint8 CTRL>
		static inline void sld(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			const uint32 n = (uint32)b & 127; // 6 bits!
			*out = (b & 64) ? 0 : (a << b);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// slw - Shift Left Word X-form
		template <uint8 CTRL>
		static inline void slw(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			const uint32 n = (uint32)b & 63; // 5 bits!
			*out = (b & 32) ? 0 : ((uint32)a << b);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// srd - Shift Right Doubleword X-form
		template <uint8 CTRL>
		static inline void srd(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			const uint32 n = (uint32)b & 127; // 6 bits!
			*out = (b & 64) ? 0 : (a >> b);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// srw - Shift Right Word X-form
		template <uint8 CTRL>
		static inline void srw(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			const uint32 n = (uint32)b & 63; // 5 bits!
			*out = (b & 32) ? 0 : ((uint32)a >> b);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// sraw - Shift Right Algebraic Word X-form
		template <uint8 CTRL>
		static inline void sraw(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			//ASM_CHECK(CTRL==0);
			const uint32 n = (uint32)b & 63;
			const uint32 v = (const uint32)a;
			const int i = (const int&)v;
			if (n <= 31)
			{
				const int64 r = i >> n; // algebraic
				*out = (const TReg&)r;
			}
			else
			{
				*out = (i < 0) ? -1 : 0;
			}
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);

		}

		// srad - Shift Right Algebraic Double Word X-form
		template <uint8 CTRL>
		static inline void srad(CpuRegs& regs, TReg* out, const TReg a, const TReg b)
		{
			ASM_CHECK(CTRL == 0);
			const uint32 n = (uint32)b & 127;
			const int64 i = (const int64&)a;
			if (n <= 63)
			{
				const int64 r = i >> n; // algebraic
				*out = (const TReg&)r;
			}
			else
			{
				*out = (i < 0) ? -1 : 0;
			}
		}

		// safe right shift for 32 bit values
		template<uint8 N>
		static inline int32 SafeShiftR32(const int32 a) { return a >> N; }
		template<>
		static inline int32 SafeShiftR32<32>(const int32 a) { return (a < 0) ? -1 : 0; }

		// safe right shift for 64 bit values
		template<uint8 N>
		static inline int64 SafeShiftR64(const int64 a) { return a >> N; }
		template<>
		static inline int64 SafeShiftR64<64>(const int64 a) { return (a < 0) ? -1 : 0; }

		// sradi - Shift Right Algebraic Doubleword Immediate XS-form
		template <uint8 CTRL, uint8 N>
		static inline void sradi(CpuRegs& regs, TReg* out, const TReg a)
		{
			ASM_CHECK(N <= 64);
			const uint8 s = (const int64&)a < 0;
			const uint64 m = HalfMask64LE<N>();
			const int64 r = SafeShiftR64<N>((const int64&)a);
			*(int64*)out = r;
			regs.XER.ca = s & ((r&~m) != 0);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// srawi - Shift Right Algebraic Word Immediate XS-form
		template <uint8 CTRL, uint8 N>
		static inline void srawi(CpuRegs& regs, TReg* out, const TReg a)
		{
			ASM_CHECK(N <= 32);
			const uint32 loA = (const uint32)a;
			const uint8 s = (const int32&)loA < 0;
			const uint32 m = HalfMask32LE<N>();
			const int32 r = SafeShiftR32<N>((const int32&)loA);
			*(int64*)out = r;
			regs.XER.ca = s & ((r&~m) != 0);
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		//---------------------------------------------------------------------------------

		// compare with immediate (signed)
		template <uint8 CR_INDEX>
		static inline void cmpwi(CpuRegs& regs, const TReg val, const uint32 imm)
		{
			ASM_CHECK(CR_INDEX >= 0 && CR_INDEX <= 7);
			const int64 a = ((const FixedReg&)val).li; // sign extended low 32-bits
			const int64 b = (const int32&)imm; // direct conversion immediate to int
			cmp::CmpSignedXER<CR_INDEX>(regs, a, b);
		}

		// compare with immediate (usigned)
		template <uint8 CR_INDEX>
		static inline void cmplwi(CpuRegs& regs, const TReg val, const uint32 imm)
		{
			ASM_CHECK(CR_INDEX >= 0 && CR_INDEX <= 7);
			const uint64 a = ((const FixedReg&)val).l; // low 32-bits
			const uint64 b = imm; // direct immediate value
			cmp::CmpUnsignedXER<CR_INDEX>(regs, a, b);
		}

		// 64bit compare with immediate (signed)
		template <uint8 CR_INDEX>
		static inline void cmpdi(CpuRegs& regs, const TReg val, const uint32 imm)
		{
			ASM_CHECK(CR_INDEX >= 0 && CR_INDEX <= 7);
			const int64 a = (const int64&)val; // directly
			const int64 b = (const int32&)imm; // sign extend
			cmp::CmpSignedXER<CR_INDEX>(regs, a, b);
		}

		// 64bit compare with immediate (usigned)
		template <uint8 CR_INDEX>
		static inline void cmpldi(CpuRegs& regs, const TReg val, const uint32 imm)
		{
			ASM_CHECK(CR_INDEX >= 0 && CR_INDEX <= 7);
			const uint64 a = (const uint64&)val;
			const uint64 b = (const uint64)imm; // extend to 64 bits
			cmp::CmpUnsignedXER<CR_INDEX>(regs, a, b);
		}

		// compare signed
		template <uint8 CR_INDEX>
		static inline void cmpw(CpuRegs& regs, const TReg val, const TReg val2)
		{
			ASM_CHECK(CR_INDEX >= 0 && CR_INDEX <= 7);
			const int64 a = ((const FixedReg&)val).li; // sign extended low 32-bits
			const int64 b = ((const FixedReg&)val2).li; // sign extended low 32-bits
			cmp::CmpSignedXER<CR_INDEX>(regs, a, b);
		}

		// compare with usigned
		template <uint8 CR_INDEX>
		static inline void cmplw(CpuRegs& regs, const TReg val, const TReg val2)
		{
			ASM_CHECK(CR_INDEX >= 0 && CR_INDEX <= 7);
			const uint64 a = ((const FixedReg&)val).l; // low 32-bits
			const uint64 b = ((const FixedReg&)val2).l; // low 32-bits
			cmp::CmpUnsignedXER<CR_INDEX>(regs, a, b);
		}

		// 64bit compare signed
		template <uint8 CR_INDEX>
		static inline void cmpd(CpuRegs& regs, const TReg val, const TReg val2)
		{
			ASM_CHECK(CR_INDEX >= 0 && CR_INDEX <= 7);
			const int64 a = (const int64&)val; // directly
			const int64 b = (const int64&)val2; // directly
			cmp::CmpSignedXER<CR_INDEX>(regs, a, b);
		}

		// 64bit compare with usigned
		template <uint8 CR_INDEX>
		static inline void cmpld(CpuRegs& regs, const TReg val, const TReg val2)
		{
			ASM_CHECK(CR_INDEX >= 0 || CR_INDEX <= 7);
			const uint64 a = (const uint64&)val;
			const uint64 b = (const uint64&)val2;
			cmp::CmpUnsignedXER<CR_INDEX>(regs, a, b);
		}

		//---------------------------------------------------------------------------------

		// cache control instruction
		template <uint8 CTRL>
		static inline void dcbt(CpuRegs& regs, const TReg* val, const TReg val2)
		{
			ASM_CHECK(CTRL == 0);
			// nothing yet
		}

		// cache control instruction
		template <uint8 CTRL>
		static inline void dcbf(CpuRegs& regs, const TReg* val, const TReg val2)
		{
			ASM_CHECK(CTRL == 0);
			// nothing yet
		}

		// cache control instruction
		template <uint8 CTRL>
		static inline void dcbz(CpuRegs& regs, const TAddr addr)
		{
			ASM_CHECK(CTRL == 0);
			const size_t cacheLineBase = (size_t)addr & ~31; // 32-byte cache lines
			memset((void*)cacheLineBase, 0, 32);
		}

		// cache control instruction
		template <uint8 CTRL>
		static inline void dcbtst(CpuRegs& regs, const TReg* val, const TReg val2)
		{
			ASM_CHECK(CTRL == 0);
			// nothing yet
		}

		//---------------------------------------------------------------------------------

		static inline uint16 clz16(uint16 x)
		{
			if (x == 0) return 16;
			uint16 n = 0;
			if ((x & 0xFF00) == 0) { n = n + 8; x = x << 8; } // 8 left bits are 0
			if ((x & 0xF000) == 0) { n = n + 4; x = x << 4; } // 4 left bits are 0
			if ((x & 0xC000) == 0) { n = n + 2, x = x << 2; }  // 110000....0 2 left bits are zero
			if ((x & 0x8000) == 0) { n = n + 1, x = x << 1; } // first left bit is zero
			return n;
		}

		static inline uint32 clz32(uint32 x)
		{
			if (x == 0) return 32;
			uint32 n = 0;
			if ((x & 0xFFFF0000) == 0) { n += 16; x = x << 16; } //1111 1111 1111 1111 0000 0000 0000 0000 // 16 bits from left are zero! so we omit 16left bits
			if ((x & 0xFF000000) == 0) { n = n + 8; x = x << 8; } // 8 left bits are 0
			if ((x & 0xF0000000) == 0) { n = n + 4; x = x << 4; } // 4 left bits are 0
			if ((x & 0xC0000000) == 0) { n = n + 2, x = x << 2; }  // 110000....0 2 left bits are zero
			if ((x & 0x80000000) == 0) { n = n + 1, x = x << 1; } // first left bit is zero
			return n;
		}

		static inline uint64 clz64(uint64 x)
		{
			if (x == 0) return 64;
			uint64 n = 0;
			if ((x & 0xFFFFFFFF00000000) == 0) { n += 32; x = x << 32; }
			if ((x & 0xFFFF000000000000) == 0) { n += 16; x = x << 16; } //1111 1111 1111 1111 0000 0000 0000 0000 // 16 bits from left are zero! so we omit 16left bits
			if ((x & 0xFF00000000000000) == 0) { n = n + 8; x = x << 8; } // 8 left bits are 0
			if ((x & 0xF000000000000000) == 0) { n = n + 4; x = x << 4; } // 4 left bits are 0
			if ((x & 0xC000000000000000) == 0) { n = n + 2, x = x << 2; }  // 110000....0 2 left bits are zero
			if ((x & 0x8000000000000000) == 0) { n = n + 1, x = x << 1; } // first left bit is zero
			return n;
		}

		// cntlzd - Count Leading Zeros Doubleword X-form
		template <uint8 CTRL>
		static inline void cntlzd(CpuRegs& regs, TReg* out, const TReg val)
		{
#if USE_HW_CLZ
			*out = __lzcnt64(val);
#else
			*out = clz64(val);
#endif			
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		// cntlzd - Count Leading Zeros Word X-form
		template <uint8 CTRL>
		static inline void cntlzw(CpuRegs& regs, TReg* out, const TReg val)
		{
#if USE_HW_CLZ
			*out = __lzcnt((uint32)val);
#else
			*out = clz32((uint32)val);
#endif
			if (CTRL) cmp::CmpLogicalXER<0>(regs, *out);
		}

		//---------------------------------------------------------------------------------

		// move to machite status register
		template <uint8 CTRL>
		static inline void mtmsrd(CpuRegs& regs, TReg* out, const TReg a)
		{
			ASM_CHECK(CTRL == 0);
			*out = a;
		}

		// move to machite status register (only the EE bit)
		template <uint8 CTRL>
		static inline void mtmsree(CpuRegs& regs, TReg* out, const TReg a)
		{
			const uint64 EEBIT_MASK = (1 << (64 - 48));
			ASM_CHECK(CTRL == 0);
			*out = (*out & ~EEBIT_MASK) | (a & EEBIT_MASK);
		}

		//---------------------------------------------------------------------------------

		template<uint8 CTRL>
		static inline void sync(CpuRegs& regs)
		{
			ASM_CHECK(CTRL == 0);
		}

		template<uint8 CTRL>
		static inline void lwsync(CpuRegs& regs)
		{
			ASM_CHECK(CTRL == 0);
		}

		template<uint8 CTRL>
		static inline void eieio(CpuRegs& regs)
		{
			ASM_CHECK(CTRL == 0);
		}

		template<uint8 FLAG>
		static inline void seteq0(CpuRegs& regs)
		{
			regs.CR[0].lt = 0;
			regs.CR[0].gt = 0;
			regs.CR[0].eq = FLAG;
			regs.CR[0].so = regs.XER.so;
		}

		//---------------------------------------------------------------------------------

		// conditional trap on word
		template <uint8 FLAGS>
		static inline void tw(CpuRegs& regs, const uint32 ip, const TReg a, const TReg b)
		{
			const uint32 u32 = (uint32)a;
			const int32 i32 = (const int32&)u32;
			const uint32 cmpu = (uint32)b;
			const int32 cmpi = (const int32&)cmpu;

			uint8 trapFlags = 0;
			if ((FLAGS & 16) && (i32 < cmpi)) trapFlags |= 1;
			if ((FLAGS & 8) && (i32 > cmpi)) trapFlags |= 2;
			if ((FLAGS & 4) && (i32 == cmpi)) trapFlags |= 4;
			if ((FLAGS & 2) && (u32 < cmpu)) trapFlags |= 8;
			if ((FLAGS & 1) && (u32 > cmpu)) trapFlags |= 16;

			if (trapFlags)
				runtime::UnhandledInterruptCall(ip, 0, regs);
		}

		// conditional trap on double word
		template <uint8 FLAGS>
		static inline void td(CpuRegs& regs, const uint32 ip, const TReg a, const TReg b)
		{
			const int64 i64 = (const int64&)a;
			const int64 cmpi = (const int64&)b;

			uint8 trapFlags = 0;
			if ((FLAGS & 16) && (i64 < cmpi)) trapFlags |= 1;
			if ((FLAGS & 8) && (i64 > cmpi)) trapFlags |= 2;
			if ((FLAGS & 4) && (i64 == cmpi)) trapFlags |= 4;
			if ((FLAGS & 2) && (a < b)) trapFlags |= 8;
			if ((FLAGS & 1) && (a > b)) trapFlags |= 16;

			if (trapFlags)
				runtime::UnhandledInterruptCall(ip, 0, regs);
		}

		// conditional trap on word
		static inline void trap(CpuRegs& regs, const uint32 ip, const TReg a, const uint32 b)
		{
			regs.INT->CallInterrupt(ip, b, regs);
		}

		// conditional trap on double word
		template <uint8 FLAGS>
		static inline void tdi(CpuRegs& regs, const uint32 ip, const TReg a, const uint32 cmp)
		{
			const int64 i64 = (const int64&)a;
			const int64 cmpi = EXTS(cmp);

			uint8 trapFlags = 0;
			if ((FLAGS & 16) && (i64 < cmpi)) trapFlags |= 1;
			if ((FLAGS & 8) && (i64 > cmpi)) trapFlags |= 2;
			if ((FLAGS & 4) && (i64 == cmpi)) trapFlags |= 4;
			if ((FLAGS & 2) && (a < cmp)) trapFlags |= 8;
			if ((FLAGS & 1) && (a > cmp)) trapFlags |= 16;

			if (trapFlags)
				regs.INT->CallInterrupt(ip, a, regs);
		}

		//---------------------------------------------------------------------------------

		// floating point control flags setup
		static inline void setCR1(CpuRegs& regs, const TFReg a)
		{
		}

		// floating point rounding
		template<uint8 CREG>
		static inline void frsp(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			*val = (float)a;
			if (CREG) setCR1(regs, *val);
		}

		// floating point convert to integer
		template<uint8 CREG>
		static inline void fctid(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			__int64 iv = (a > 0.0f) ? (__int64)(a + 0.5) : (__int64)(a - 0.5);
			*val = *(TFReg*)&iv;
			if (CREG) setCR1(regs, *val);
		}

		// floating point convert to integer
		template<uint8 CREG>
		static inline void fctiw(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			__int64 iv = (a > 0.0f) ? (__int64)(a + 0.5) : (__int64)(a - 0.5);
			*val = *(TFReg*)&iv;
			if (CREG) setCR1(regs, *val);
		}

		// floating point convert to integer with rounding towards zero
		template<uint8 CREG>
		static inline void fctidz(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			__int64 iv = (__int64)a;
			*val = *(TFReg*)&iv;
			if (CREG) setCR1(regs, *val);
		}

		// floating point convert to integer
		template<uint8 CREG>
		static inline void fctiwz(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			__int64 iv = (int)a;
			*val = *(TFReg*)&iv;
			if (CREG) setCR1(regs, *val);
		}

		// floating point convert from integer
		template<uint8 CREG>
		static inline void fcfid(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			*val = (TFReg) *(__int64*)&a;
			if (CREG) setCR1(regs, *val);
		}


		//---------------------------------------------------------------------------------

		// floating point abs
		template<uint8 CREG>
		static inline void fmr(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			ASM_CHECK(CREG == 0);
			*val = a;
		}

		// floating point abs
		template<uint8 CREG>
		static inline void fabs(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			*val = (a < 0.0) ? -a : a;
			if (CREG) setCR1(regs, *val);
		}

		// floating point negative
		template<uint8 CREG>
		static inline void fneg(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			*val = -a;
			if (CREG) setCR1(regs, *val);
		}

		// floating point square root
		template<uint8 CREG>
		static inline void fsqrt(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			*val = sqrt(a);
			if (CREG) setCR1(regs, *val);
		}

		// floating point inverse square root
		template<uint8 CREG>
		static inline void frsqrtx(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			*val = (a > 0.0f) ? 1.0f / sqrt(a) : 0.0f;
			if (CREG) setCR1(regs, *val);
		}

		// floating point square root
		template<uint8 CREG>
		static inline void fsqrts(CpuRegs& regs, TFReg* val, const TFReg a)
		{
			*val = (TFReg)sqrtf((float)a);
			if (CREG) setCR1(regs, *val);
		}

		// fsel
		template<uint8 CREG>
		static inline void fsel(CpuRegs& regs, TFReg* val, const TFReg x, const TFReg a, const TFReg b)
		{
			*val = (x >= 0) ? a : b;
			if (CREG) setCR1(regs, *val);
		}

		// fnmsub
		template<uint8 CREG>
		static inline void fnmsub(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b, const TFReg c)
		{
			*val = -((a*b) - c);
			if (CREG) setCR1(regs, *val);
		}

		// fnmsubs
		template<uint8 CREG>
		static inline void fnmsubs(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b, const TFReg c)
		{
			*val = (TFReg)-((((float)a) * ((float)b)) - ((float)c));
			if (CREG) setCR1(regs, *val);
		}

		// fmadd
		template<uint8 CREG>
		static inline void fmadd(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b, const TFReg c)
		{
			*val = (a*b) + c;
			if (CREG) setCR1(regs, *val);
		}

		// fmsub
		template<uint8 CREG>
		static inline void fmsub(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b, const TFReg c)
		{
			*val = (a*b) - c;
			if (CREG) setCR1(regs, *val);
		}

		// fmadds
		template<uint8 CREG>
		static inline void fmadds(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b, const TFReg c)
		{
			*val = (TFReg)(((float)a) * ((float)b)) + ((float)c);
			if (CREG) setCR1(regs, *val);
		}

		// fmsubs
		template<uint8 CREG>
		static inline void fmsubs(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b, const TFReg c)
		{
			*val = (TFReg)(((float)a) * ((float)b)) - ((float)c);
			if (CREG) setCR1(regs, *val);
		}

		// fnmadds
		template<uint8 CREG>
		static inline void fnmadds(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b, const TFReg c)
		{
			*val = -(TFReg)(((float)a) * ((float)b)) + ((float)c);
			if (CREG) setCR1(regs, *val);
		}

		// fmul
		template<uint8 CREG>
		static inline void fmul(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b)
		{
			*val = a * b;
			if (CREG) setCR1(regs, *val);
		}

		// fmuls
		template<uint8 CREG>
		static inline void fmuls(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b)
		{
			*val = ((float)a) * ((float)b);
			if (CREG) setCR1(regs, *val);
		}

		// fdiv
		template<uint8 CREG>
		static inline void fdiv(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b)
		{
			*val = a / b;
			if (CREG) setCR1(regs, *val);
		}

		// fdivs
		template<uint8 CREG>
		static inline void fdivs(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b)
		{
			*val = ((float)a) / ((float)b);
			if (CREG) setCR1(regs, *val);
		}

		// fadd
		template<uint8 CREG>
		static inline void fadd(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b)
		{
			*val = a + b;
			if (CREG) setCR1(regs, *val);
		}

		// fadds
		template<uint8 CREG>
		static inline void fadds(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b)
		{
			*val = ((float)a) + ((float)b);
			if (CREG) setCR1(regs, *val);
		}

		// fsub
		template<uint8 CREG>
		static inline void fsub(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b)
		{
			*val = a - b;
			if (CREG) setCR1(regs, *val);
		}

		// fsubs
		template<uint8 CREG>
		static inline void fsubs(CpuRegs& regs, TFReg* val, const TFReg a, const TFReg b)
		{
			*val = ((float)a) - ((float)b);
			if (CREG) setCR1(regs, *val);
		}


		// floating point comparision
		template <uint8 CR_INDEX>
		static inline void fcmpu(CpuRegs& regs, const TFReg a, const TFReg b)
		{
			ASM_CHECK(CR_INDEX >= 0 || CR_INDEX <= 7);

			ControlReg& cr = regs.CR[CR_INDEX];
			if (_isnan(a) || _isnan(b))
			{
				cr.so = 1;
				cr.lt = cr.gt = cr.eq = 0;
			}
			else if (a < b)
			{
				cr.lt = 1;
				cr.gt = 0;
				cr.eq = 0;
				cr.so = 0;
			}
			else if (a > b)
			{
				cr.lt = 0;
				cr.gt = 1;
				cr.eq = 0;
				cr.so = 0;
			}
			else
			{
				cr.lt = 0;
				cr.gt = 0;
				cr.eq = 1;
				cr.so = 0;
			}

			//regs.FPCC = cr;
		}

		//---------------------------------------------------------------------------------

		template <uint8 CTRL>
		static inline void mftb(CpuRegs& regs, TReg* out, int a, int b)
		{
			if (a == 12 && b == 8)
			{
				LARGE_INTEGER val;
				QueryPerformanceCounter(&val);
				*out = (TReg)val.QuadPart;
			}
			else
			{
				*out = 0;
			}
		}

		template <uint8 CTRL>
		static inline void mfcr(CpuRegs& regs, TReg* out)
		{
			ASM_CHECK(CTRL == 0);
			const uint32 mask = regs.GetControlRegsFlags();
			*out = mask;
		}

		template <uint8 CTRL>
		static inline void mtcrf(CpuRegs& regs, const uint8 mask, const TReg val)
		{
			ASM_CHECK(CTRL == 0);
			if (mask & 0x80) regs.CR[0].Set((val >> 28) & 0xF);
			if (mask & 0x40) regs.CR[1].Set((val >> 24) & 0xF);
			if (mask & 0x20) regs.CR[2].Set((val >> 20) & 0xF);
			if (mask & 0x10) regs.CR[3].Set((val >> 16) & 0xF);
			if (mask & 0x08) regs.CR[4].Set((val >> 12) & 0xF);
			if (mask & 0x04) regs.CR[5].Set((val >> 8) & 0xF);
			if (mask & 0x02) regs.CR[6].Set((val >> 4) & 0xF);
			if (mask & 0x01) regs.CR[7].Set((val >> 0) & 0xF);
		}

		template <uint8 CTRL>
		static inline void mfocrf(CpuRegs& regs, const uint32 mask, TReg& out)
		{
			ASM_CHECK(CTRL == 0);
			const uint32 localMask = 0xF0000000 >> (4 * mask);
			out = regs.GetControlRegsFlags() & localMask;
		}

		template <uint8 CTRL>
		static inline void mffs(CpuRegs& regs, TFReg* out)
		{
			ASM_CHECK(CTRL == 0);

			union
			{
				TReg u;
				TFReg f;
			} x;

			x.u = regs.FPSCR;
			*out = x.f;
		}

		template <uint8 CTRL>
		static inline void mtfsf(CpuRegs& regs, const uint8 mask, const TFReg val)
		{
			ASM_CHECK(CTRL == 0);

			union
			{
				TReg u;
				TFReg f;
			} x;

			x.f = val;
			regs.FPSCR = (x.u) & 0xFFFFFFFF;
		}

		//---------------------------------------------------------------------------------

		template <uint8 CTRL, uint8 SEL>
		static inline void vspltw(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			ASM_CHECK(SEL <= 3);
			out->AsUint32<0>() = a.AsUint32<SEL>();
			out->AsUint32<1>() = a.AsUint32<SEL>();
			out->AsUint32<2>() = a.AsUint32<SEL>();
			out->AsUint32<3>() = a.AsUint32<SEL>();
		}

		template <uint8 CTRL, uint8 SEL>
		static inline void vsplth(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			ASM_CHECK(SEL <= 7);
			out->AsUint16<0>() = a.AsUint16<SEL>();
			out->AsUint16<1>() = a.AsUint16<SEL>();
			out->AsUint16<2>() = a.AsUint16<SEL>();
			out->AsUint16<3>() = a.AsUint16<SEL>();
			out->AsUint16<4>() = a.AsUint16<SEL>();
			out->AsUint16<5>() = a.AsUint16<SEL>();
			out->AsUint16<6>() = a.AsUint16<SEL>();
			out->AsUint16<7>() = a.AsUint16<SEL>();
		}

		template <uint8 CTRL, uint8 SEL>
		static inline void vspltb(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			ASM_CHECK(SEL <= 15);
			out->AsUint8<0>() = a.AsUint8<SEL>();
			out->AsUint8<1>() = a.AsUint8<SEL>();
			out->AsUint8<2>() = a.AsUint8<SEL>();
			out->AsUint8<3>() = a.AsUint8<SEL>();
			out->AsUint8<4>() = a.AsUint8<SEL>();
			out->AsUint8<5>() = a.AsUint8<SEL>();
			out->AsUint8<6>() = a.AsUint8<SEL>();
			out->AsUint8<7>() = a.AsUint8<SEL>();
			out->AsUint8<8>() = a.AsUint8<SEL>();
			out->AsUint8<9>() = a.AsUint8<SEL>();
			out->AsUint8<10>() = a.AsUint8<SEL>();
			out->AsUint8<11>() = a.AsUint8<SEL>();
			out->AsUint8<12>() = a.AsUint8<SEL>();
			out->AsUint8<13>() = a.AsUint8<SEL>();
			out->AsUint8<14>() = a.AsUint8<SEL>();
			out->AsUint8<15>() = a.AsUint8<SEL>();
		}

		template <uint8 CTRL>
		static inline void vor(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = a.AsUint32<0>() | b.AsUint32<0>();
			out->AsUint32<1>() = a.AsUint32<1>() | b.AsUint32<1>();
			out->AsUint32<2>() = a.AsUint32<2>() | b.AsUint32<2>();
			out->AsUint32<3>() = a.AsUint32<3>() | b.AsUint32<3>();
		}

		template <uint8 CTRL>
		static inline void vxor(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = a.AsUint32<0>() ^ b.AsUint32<0>();
			out->AsUint32<1>() = a.AsUint32<1>() ^ b.AsUint32<1>();
			out->AsUint32<2>() = a.AsUint32<2>() ^ b.AsUint32<2>();
			out->AsUint32<3>() = a.AsUint32<3>() ^ b.AsUint32<3>();
		}

		template <uint8 CTRL>
		static inline void vnor(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = ~(a.AsUint32<0>() | b.AsUint32<0>());
			out->AsUint32<1>() = ~(a.AsUint32<1>() | b.AsUint32<1>());
			out->AsUint32<2>() = ~(a.AsUint32<2>() | b.AsUint32<2>());
			out->AsUint32<3>() = ~(a.AsUint32<3>() | b.AsUint32<3>());
		}

		template <uint8 CTRL>
		static inline void vand(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = a.AsUint32<0>() & b.AsUint32<0>();
			out->AsUint32<1>() = a.AsUint32<1>() & b.AsUint32<1>();
			out->AsUint32<2>() = a.AsUint32<2>() & b.AsUint32<2>();
			out->AsUint32<3>() = a.AsUint32<3>() & b.AsUint32<3>();
		}

		template <uint8 CTRL>
		static inline void vandc(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = a.AsUint32<0>() & ~b.AsUint32<0>();
			out->AsUint32<1>() = a.AsUint32<1>() & ~b.AsUint32<1>();
			out->AsUint32<2>() = a.AsUint32<2>() & ~b.AsUint32<2>();
			out->AsUint32<3>() = a.AsUint32<3>() & ~b.AsUint32<3>();
		}

		template <uint8 CTRL, uint8 MASK, uint8 ROT>
		static inline void vrlimi128(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			if (MASK & 8) out->AsUint32<0>() = a.AsUint32< (0 + ROT) & 3 >();
			if (MASK & 4) out->AsUint32<1>() = a.AsUint32< (1 + ROT) & 3 >();
			if (MASK & 2) out->AsUint32<2>() = a.AsUint32< (2 + ROT) & 3 >();
			if (MASK & 1) out->AsUint32<3>() = a.AsUint32< (3 + ROT) & 3 >();
		}

		template <uint8 CTRL>
		static inline void vmrghw(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = a.AsUint32<0>();
			out->AsUint32<1>() = b.AsUint32<0>();
			out->AsUint32<2>() = a.AsUint32<1>();
			out->AsUint32<3>() = b.AsUint32<1>();
		}

		template <uint8 CTRL>
		static inline void vmrglw(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = a.AsUint32<2>();
			out->AsUint32<1>() = b.AsUint32<2>();
			out->AsUint32<2>() = a.AsUint32<3>();
			out->AsUint32<3>() = b.AsUint32<3>();
		}

		static const uint8 Reindex[32] = { 3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12, 19,18,17,16, 23,22,21,20, 27,26,25,24, 31,30,29,28 };
		static const uint8 ReindexWords[16] = { 1,0, 3,2, 5,4, 7,6, 9,8, 11,10, 13,12, 15,14 };

		static inline uint8 vperm_helper(const TVReg& a, const TVReg& b, uint8 index)
		{
			return (index & 16) ? b.u8[index & 15] : a.u8[index & 15];
		}

		template <uint8 CTRL>
		static inline void vperm(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b, const TVReg m)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint8<0>() = vperm_helper(a, b, Reindex[m.u8[Reindex[0]]]);
			out->AsUint8<1>() = vperm_helper(a, b, Reindex[m.u8[Reindex[1]]]);
			out->AsUint8<2>() = vperm_helper(a, b, Reindex[m.u8[Reindex[2]]]);
			out->AsUint8<3>() = vperm_helper(a, b, Reindex[m.u8[Reindex[3]]]);
			out->AsUint8<4>() = vperm_helper(a, b, Reindex[m.u8[Reindex[4]]]);
			out->AsUint8<5>() = vperm_helper(a, b, Reindex[m.u8[Reindex[5]]]);
			out->AsUint8<6>() = vperm_helper(a, b, Reindex[m.u8[Reindex[6]]]);
			out->AsUint8<7>() = vperm_helper(a, b, Reindex[m.u8[Reindex[7]]]);
			out->AsUint8<8>() = vperm_helper(a, b, Reindex[m.u8[Reindex[8]]]);
			out->AsUint8<9>() = vperm_helper(a, b, Reindex[m.u8[Reindex[9]]]);
			out->AsUint8<10>() = vperm_helper(a, b, Reindex[m.u8[Reindex[10]]]);
			out->AsUint8<11>() = vperm_helper(a, b, Reindex[m.u8[Reindex[11]]]);
			out->AsUint8<12>() = vperm_helper(a, b, Reindex[m.u8[Reindex[12]]]);
			out->AsUint8<13>() = vperm_helper(a, b, Reindex[m.u8[Reindex[13]]]);
			out->AsUint8<14>() = vperm_helper(a, b, Reindex[m.u8[Reindex[14]]]);
			out->AsUint8<15>() = vperm_helper(a, b, Reindex[m.u8[Reindex[15]]]);
		}

		template <uint8 CTRL>
		static inline void vmrglb(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint8<0>() = a.AsUint8<0>();
			out->AsUint8<1>() = b.AsUint8<0>();
			out->AsUint8<2>() = a.AsUint8<1>();
			out->AsUint8<3>() = b.AsUint8<1>();
			out->AsUint8<4>() = a.AsUint8<2>();
			out->AsUint8<5>() = b.AsUint8<2>();
			out->AsUint8<6>() = a.AsUint8<3>();
			out->AsUint8<7>() = b.AsUint8<3>();
			out->AsUint8<8>() = a.AsUint8<4>();
			out->AsUint8<9>() = b.AsUint8<4>();
			out->AsUint8<10>() = a.AsUint8<5>();
			out->AsUint8<11>() = b.AsUint8<5>();
			out->AsUint8<12>() = a.AsUint8<6>();
			out->AsUint8<13>() = b.AsUint8<6>();
			out->AsUint8<14>() = a.AsUint8<7>();
			out->AsUint8<15>() = b.AsUint8<7>();
		}

		template <uint8 CTRL>
		static inline void vmrghb(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint8<0>() = a.AsUint8<8>();
			out->AsUint8<1>() = b.AsUint8<8>();
			out->AsUint8<2>() = a.AsUint8<9>();
			out->AsUint8<3>() = b.AsUint8<9>();
			out->AsUint8<4>() = a.AsUint8<10>();
			out->AsUint8<5>() = b.AsUint8<10>();
			out->AsUint8<6>() = a.AsUint8<11>();
			out->AsUint8<7>() = b.AsUint8<11>();
			out->AsUint8<8>() = a.AsUint8<12>();
			out->AsUint8<9>() = b.AsUint8<12>();
			out->AsUint8<10>() = a.AsUint8<13>();
			out->AsUint8<11>() = b.AsUint8<13>();
			out->AsUint8<12>() = a.AsUint8<14>();
			out->AsUint8<13>() = b.AsUint8<14>();
			out->AsUint8<14>() = a.AsUint8<15>();
			out->AsUint8<15>() = b.AsUint8<15>();
		}

		template <uint8 SH>
		static inline uint8 vsldoi_helper(const TVReg& a, const TVReg& b)
		{
			return (SH < 16) ? a.AsUint8<SH>() : b.AsUint8<SH & 15>();
		}

		template <uint8 CTRL, uint8 SHIFT>
		static inline void vsldoi(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint8<0>() = vsldoi_helper<SHIFT + 0>(a, b);
			out->AsUint8<1>() = vsldoi_helper<SHIFT + 1>(a, b);
			out->AsUint8<2>() = vsldoi_helper<SHIFT + 2>(a, b);
			out->AsUint8<3>() = vsldoi_helper<SHIFT + 3>(a, b);
			out->AsUint8<4>() = vsldoi_helper<SHIFT + 4>(a, b);
			out->AsUint8<5>() = vsldoi_helper<SHIFT + 5>(a, b);
			out->AsUint8<6>() = vsldoi_helper<SHIFT + 6>(a, b);
			out->AsUint8<7>() = vsldoi_helper<SHIFT + 7>(a, b);
			out->AsUint8<8>() = vsldoi_helper<SHIFT + 8>(a, b);
			out->AsUint8<9>() = vsldoi_helper<SHIFT + 9>(a, b);
			out->AsUint8<10>() = vsldoi_helper<SHIFT + 10>(a, b);
			out->AsUint8<11>() = vsldoi_helper<SHIFT + 11>(a, b);
			out->AsUint8<12>() = vsldoi_helper<SHIFT + 12>(a, b);
			out->AsUint8<13>() = vsldoi_helper<SHIFT + 13>(a, b);
			out->AsUint8<14>() = vsldoi_helper<SHIFT + 14>(a, b);
			out->AsUint8<15>() = vsldoi_helper<SHIFT + 15>(a, b);
		}

		template <uint8 CTRL>
		static inline void vslh(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint16<0>() = a.AsUint16<0>() << (b.AsUint8<0>() & 0xF);
			out->AsUint16<1>() = a.AsUint16<1>() << (b.AsUint8<1>() & 0xF);
			out->AsUint16<2>() = a.AsUint16<2>() << (b.AsUint8<2>() & 0xF);
			out->AsUint16<3>() = a.AsUint16<3>() << (b.AsUint8<3>() & 0xF);
			out->AsUint16<4>() = a.AsUint16<4>() << (b.AsUint8<4>() & 0xF);
			out->AsUint16<5>() = a.AsUint16<5>() << (b.AsUint8<5>() & 0xF);
			out->AsUint16<6>() = a.AsUint16<6>() << (b.AsUint8<6>() & 0xF);
			out->AsUint16<7>() = a.AsUint16<7>() << (b.AsUint8<7>() & 0xF);
		}

		template <uint8 CTRL>
		static inline void vsrh(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint16<0>() = a.AsUint16<0>() >> (b.AsUint8<0>() & 0xF);
			out->AsUint16<1>() = a.AsUint16<1>() >> (b.AsUint8<1>() & 0xF);
			out->AsUint16<2>() = a.AsUint16<2>() >> (b.AsUint8<2>() & 0xF);
			out->AsUint16<3>() = a.AsUint16<3>() >> (b.AsUint8<3>() & 0xF);
			out->AsUint16<4>() = a.AsUint16<4>() >> (b.AsUint8<4>() & 0xF);
			out->AsUint16<5>() = a.AsUint16<5>() >> (b.AsUint8<5>() & 0xF);
			out->AsUint16<6>() = a.AsUint16<6>() >> (b.AsUint8<6>() & 0xF);
			out->AsUint16<7>() = a.AsUint16<7>() >> (b.AsUint8<7>() & 0xF);
		}

		template <uint8 CTRL>
		static inline void vslb(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint8<0 >() = a.AsUint8<0 >() << (b.AsUint8<0 >() & 0x7);
			out->AsUint8<1 >() = a.AsUint8<1 >() << (b.AsUint8<1 >() & 0x7);
			out->AsUint8<2 >() = a.AsUint8<2 >() << (b.AsUint8<2 >() & 0x7);
			out->AsUint8<3 >() = a.AsUint8<3 >() << (b.AsUint8<3 >() & 0x7);
			out->AsUint8<4 >() = a.AsUint8<4 >() << (b.AsUint8<4 >() & 0x7);
			out->AsUint8<5 >() = a.AsUint8<5 >() << (b.AsUint8<5 >() & 0x7);
			out->AsUint8<6 >() = a.AsUint8<6 >() << (b.AsUint8<6 >() & 0x7);
			out->AsUint8<7 >() = a.AsUint8<7 >() << (b.AsUint8<7 >() & 0x7);
			out->AsUint8<8 >() = a.AsUint8<8 >() << (b.AsUint8<8 >() & 0x7);
			out->AsUint8<9 >() = a.AsUint8<9 >() << (b.AsUint8<9 >() & 0x7);
			out->AsUint8<10>() = a.AsUint8<10>() << (b.AsUint8<10>() & 0x7);
			out->AsUint8<11>() = a.AsUint8<11>() << (b.AsUint8<11>() & 0x7);
			out->AsUint8<12>() = a.AsUint8<12>() << (b.AsUint8<12>() & 0x7);
			out->AsUint8<13>() = a.AsUint8<13>() << (b.AsUint8<13>() & 0x7);
			out->AsUint8<14>() = a.AsUint8<14>() << (b.AsUint8<14>() & 0x7);
			out->AsUint8<15>() = a.AsUint8<15>() << (b.AsUint8<15>() & 0x7);
		}

		template <uint8 CTRL>
		static inline void vsrb(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint8<0 >() = a.AsUint8<0>() >> (b.AsUint8<0>() & 0x7);
			out->AsUint8<1 >() = a.AsUint8<1>() >> (b.AsUint8<1>() & 0x7);
			out->AsUint8<2 >() = a.AsUint8<2>() >> (b.AsUint8<2>() & 0x7);
			out->AsUint8<3 >() = a.AsUint8<3>() >> (b.AsUint8<3>() & 0x7);
			out->AsUint8<4 >() = a.AsUint8<4>() >> (b.AsUint8<4>() & 0x7);
			out->AsUint8<5 >() = a.AsUint8<5>() >> (b.AsUint8<5>() & 0x7);
			out->AsUint8<6 >() = a.AsUint8<6>() >> (b.AsUint8<6>() & 0x7);
			out->AsUint8<7 >() = a.AsUint8<7>() >> (b.AsUint8<7>() & 0x7);
			out->AsUint8<8 >() = a.AsUint8<8>() >> (b.AsUint8<8>() & 0x7);
			out->AsUint8<9 >() = a.AsUint8<9>() >> (b.AsUint8<9>() & 0x7);
			out->AsUint8<10>() = a.AsUint8<10>() >> (b.AsUint8<10>() & 0x7);
			out->AsUint8<11>() = a.AsUint8<11>() >> (b.AsUint8<11>() & 0x7);
			out->AsUint8<12>() = a.AsUint8<12>() >> (b.AsUint8<12>() & 0x7);
			out->AsUint8<13>() = a.AsUint8<13>() >> (b.AsUint8<13>() & 0x7);
			out->AsUint8<14>() = a.AsUint8<14>() >> (b.AsUint8<14>() & 0x7);
			out->AsUint8<15>() = a.AsUint8<15>() >> (b.AsUint8<15>() & 0x7);
		}

		template <uint8 CTRL>
		static inline void vsel(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b, const TVReg m)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = (a.AsUint32<0>() & ~m.AsUint32<0>()) | (b.AsUint32<0>() & m.AsUint32<0>());
			out->AsUint32<1>() = (a.AsUint32<1>() & ~m.AsUint32<1>()) | (b.AsUint32<1>() & m.AsUint32<1>());
			out->AsUint32<2>() = (a.AsUint32<2>() & ~m.AsUint32<2>()) | (b.AsUint32<2>() & m.AsUint32<2>());
			out->AsUint32<3>() = (a.AsUint32<3>() & ~m.AsUint32<3>()) | (b.AsUint32<3>() & m.AsUint32<3>());
		}

		template <uint8 CTRL, uint8 val>
		static inline void vpermwi128(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = a.AsUint32<(val >> 6) & 3>();
			out->AsUint32<1>() = a.AsUint32<(val >> 4) & 3>();
			out->AsUint32<2>() = a.AsUint32<(val >> 2) & 3>();
			out->AsUint32<3>() = a.AsUint32<(val >> 0) & 3>();
		}

		template< int N >
		struct CompareHelper
		{
			template< typename T >
			static inline bool AllEqual(const T& a, const T& b)
			{
				for (int i = 0; i < N; ++i)
				{
					if (a[i] != b[i])
						return false;
				}

				return true;
			}

			template< typename T >
			static inline bool AllNotEqual(const T& a, const T& b)
			{
				for (int i = 0; i < N; ++i)
				{
					if (a[i] != b[i])
						return false;
				}

				return true;
			}

			template< typename T, typename R >
			static inline bool AllSet(const T& a, const R& val)
			{
				for (int i = 0; i < N; ++i)
				{
					if (a[i] != val)
						return false;
				}

				return true;
			}

			template< typename R, typename T, typename Val >
			static inline void SetIfEqual(R& out, const T& a, const T& b, Val valToSet)
			{
				for (int i = 0; i < N; ++i)
				{
					out[i] = (a[i] == b[i]) ? valToSet : 0;
				}
			}

			template< typename R, typename T, typename Val >
			static inline void SetIfNotEqual(R& out, const T& a, const T& b, Val valToSet)
			{
				for (int i = 0; i < N; ++i)
				{
					out[i] = (a[i] == b[i]) ? valToSet : 0;
				}
			}

			template< typename R, typename T, typename Val >
			static inline void SetIfLess(R& out, const T& a, const T& b, Val valToSet)
			{
				for (int i = 0; i < N; ++i)
				{
					out[i] = (a[i] < b[i]) ? valToSet : 0;
				}
			}

			template< typename R, typename T, typename Val >
			static inline void SetIfLessEqual(R& out, const T& a, const T& b, Val valToSet)
			{
				for (int i = 0; i < N; ++i)
				{
					out[i] = (a[i] >= b[i]) ? valToSet : 0;
				}
			}

			template< typename R, typename T, typename Val >
			static inline void SetIfGreater(R& out, const T& a, const T& b, Val valToSet)
			{
				for (int i = 0; i < N; ++i)
				{
					out[i] = (a[i] > b[i]) ? valToSet : 0;
				}
			}

			template< typename R, typename T, typename Val >
			static inline void SetIfGreaterEqual(R& out, const T& a, const T& b, Val valToSet)
			{
				for (int i = 0; i < N; ++i)
				{
					out[i] = (a[i] >= b[i]) ? valToSet : 0;
				}
			}

		}; // CompareHelper

		template <uint8 CTRL>
		static inline void vcmpequb(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			CompareHelper<16>::SetIfEqual(out->u8, a.u8, b.u8, 0xFF);

			if (CTRL == 1)
			{
				const bool allEqual = CompareHelper<16>::AllEqual(a.u8, b.u8);
				const bool allNotEqual = CompareHelper<16>::AllNotEqual(a.u8, b.u8);
				regs.CR[6].so = 0;
				regs.CR[6].eq = allNotEqual ? 1 : 0;
				regs.CR[6].gt = 0;
				regs.CR[6].lt = allEqual ? 1 : 0;
			}
		}

		template <uint8 CTRL>
		static inline void vcmpequw(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			out->AsUint32<0>() = (a.AsUint32<0>() == b.AsUint32<0>()) ? 0xFFFFFFFF : 0;
			out->AsUint32<1>() = (a.AsUint32<1>() == b.AsUint32<1>()) ? 0xFFFFFFFF : 0;
			out->AsUint32<2>() = (a.AsUint32<2>() == b.AsUint32<2>()) ? 0xFFFFFFFF : 0;
			out->AsUint32<3>() = (a.AsUint32<3>() == b.AsUint32<3>()) ? 0xFFFFFFFF : 0;

			if (CTRL == 1)
			{
				const bool allEqual = CompareHelper<4>::AllSet(out->u32, 0xFFFFFFFF);
				const bool allNotEqual = CompareHelper<4>::AllSet(out->u32, 0x0);
				regs.CR[6].so = 0;
				regs.CR[6].eq = allNotEqual ? 1 : 0;
				regs.CR[6].gt = 0;
				regs.CR[6].lt = allEqual ? 1 : 0;
			}
		}

		template <uint8 CTRL>
		static inline void vcmpeqfp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			out->AsUint32<0>() = (a.AsFloat<0>() == b.AsFloat<0>()) ? 0xFFFFFFFF : 0;
			out->AsUint32<1>() = (a.AsFloat<1>() == b.AsFloat<1>()) ? 0xFFFFFFFF : 0;
			out->AsUint32<2>() = (a.AsFloat<2>() == b.AsFloat<2>()) ? 0xFFFFFFFF : 0;
			out->AsUint32<3>() = (a.AsFloat<3>() == b.AsFloat<3>()) ? 0xFFFFFFFF : 0;

			if (CTRL == 1)
			{
				const bool allEqual = CompareHelper<4>::AllSet(out->u32, 0xFFFFFFFF);
				const bool allNotEqual = CompareHelper<4>::AllSet(out->u32, 0x0);
				regs.CR[6].so = 0;
				regs.CR[6].eq = allNotEqual ? 1 : 0;
				regs.CR[6].gt = 0;
				regs.CR[6].lt = allEqual ? 1 : 0;
			}
		}

		template <uint8 CTRL>
		static inline void vcmpgefp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			out->AsUint32<0>() = (a.AsFloat<0>() >= b.AsFloat<0>()) ? 0xFFFFFFFF : 0;
			out->AsUint32<1>() = (a.AsFloat<1>() >= b.AsFloat<1>()) ? 0xFFFFFFFF : 0;
			out->AsUint32<2>() = (a.AsFloat<2>() >= b.AsFloat<2>()) ? 0xFFFFFFFF : 0;
			out->AsUint32<3>() = (a.AsFloat<3>() >= b.AsFloat<3>()) ? 0xFFFFFFFF : 0;

			if (CTRL == 1)
			{
				const bool allEqual = CompareHelper<4>::AllSet(out->u32, 0xFFFFFFFF);
				const bool allNotEqual = CompareHelper<4>::AllSet(out->u32, 0x0);
				regs.CR[6].so = 0;
				regs.CR[6].eq = allNotEqual ? 1 : 0;
				regs.CR[6].gt = 0;
				regs.CR[6].lt = allEqual ? 1 : 0;
			}
		}

		template <uint8 CTRL>
		static inline void vcmpgtub(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			CompareHelper<16>::SetIfGreater(out->u8, a.u8, b.u8, 0xFF);
		}

		static inline uint8 vsat8(CpuRegs& regs, uint32 val)
		{
			if (val > 255)
			{
				regs.SAT = 1;
				val = 255;
			}
			return (uint8)val;
		}

		static inline uint8 vsat8i(CpuRegs& regs, int16 val)
		{
			if (val > 255)
			{
				regs.SAT = 1;
				val = 255;
			}
			else if (val < 0)
			{
				regs.SAT = 1;
				val = 0;
			}
			return (uint8)val;
		}

		template <uint8 CTRL>
		static inline void vaddubs(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint8<0 >() = vsat8(regs, (uint32)a.AsUint8<0 >() + (uint32)b.AsUint8<0 >());
			out->AsUint8<1 >() = vsat8(regs, (uint32)a.AsUint8<1 >() + (uint32)b.AsUint8<1 >());
			out->AsUint8<2 >() = vsat8(regs, (uint32)a.AsUint8<2 >() + (uint32)b.AsUint8<2 >());
			out->AsUint8<3 >() = vsat8(regs, (uint32)a.AsUint8<3 >() + (uint32)b.AsUint8<3 >());
			out->AsUint8<4 >() = vsat8(regs, (uint32)a.AsUint8<4 >() + (uint32)b.AsUint8<4 >());
			out->AsUint8<5 >() = vsat8(regs, (uint32)a.AsUint8<5 >() + (uint32)b.AsUint8<5 >());
			out->AsUint8<6 >() = vsat8(regs, (uint32)a.AsUint8<6 >() + (uint32)b.AsUint8<6 >());
			out->AsUint8<7 >() = vsat8(regs, (uint32)a.AsUint8<7 >() + (uint32)b.AsUint8<7 >());
			out->AsUint8<8 >() = vsat8(regs, (uint32)a.AsUint8<8 >() + (uint32)b.AsUint8<8 >());
			out->AsUint8<9 >() = vsat8(regs, (uint32)a.AsUint8<9 >() + (uint32)b.AsUint8<9 >());
			out->AsUint8<10>() = vsat8(regs, (uint32)a.AsUint8<10>() + (uint32)b.AsUint8<10>());
			out->AsUint8<11>() = vsat8(regs, (uint32)a.AsUint8<11>() + (uint32)b.AsUint8<11>());
			out->AsUint8<12>() = vsat8(regs, (uint32)a.AsUint8<12>() + (uint32)b.AsUint8<12>());
			out->AsUint8<13>() = vsat8(regs, (uint32)a.AsUint8<13>() + (uint32)b.AsUint8<13>());
			out->AsUint8<14>() = vsat8(regs, (uint32)a.AsUint8<14>() + (uint32)b.AsUint8<14>());
			out->AsUint8<15>() = vsat8(regs, (uint32)a.AsUint8<15>() + (uint32)b.AsUint8<15>());
		}

		template <uint8 CTRL>
		static inline void vadduhm(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint16<0>() = a.AsUint16<0>() + b.AsUint16<0>();
			out->AsUint16<1>() = a.AsUint16<1>() + b.AsUint16<1>();
			out->AsUint16<2>() = a.AsUint16<2>() + b.AsUint16<2>();
			out->AsUint16<3>() = a.AsUint16<3>() + b.AsUint16<3>();
			out->AsUint16<4>() = a.AsUint16<4>() + b.AsUint16<4>();
			out->AsUint16<5>() = a.AsUint16<5>() + b.AsUint16<5>();
			out->AsUint16<6>() = a.AsUint16<6>() + b.AsUint16<6>();
			out->AsUint16<7>() = a.AsUint16<7>() + b.AsUint16<7>();
		}

		template <uint8 CTRL>
		static inline void vsubuhm(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsInt16<0>() = a.AsInt16<0>() - b.AsInt16<0>();
			out->AsInt16<1>() = a.AsInt16<1>() - b.AsInt16<1>();
			out->AsInt16<2>() = a.AsInt16<2>() - b.AsInt16<2>();
			out->AsInt16<3>() = a.AsInt16<3>() - b.AsInt16<3>();
			out->AsInt16<4>() = a.AsInt16<4>() - b.AsInt16<4>();
			out->AsInt16<5>() = a.AsInt16<5>() - b.AsInt16<5>();
			out->AsInt16<6>() = a.AsInt16<6>() - b.AsInt16<6>();
			out->AsInt16<7>() = a.AsInt16<7>() - b.AsInt16<7>();
		}

		static inline uint16 vsat16(CpuRegs& regs, int32 val)
		{
			if (val > 65535)
			{
				regs.SAT = 1;
				val = 65535;
			}
			else if (val < 0)
			{
				regs.SAT = 1;
				val = 0;
			}
			return (uint16)val;
		}

		static inline int16 vsat16i(CpuRegs& regs, int32 val)
		{
			if (val > SHORT_MAX)
			{
				regs.SAT = 1;
				val = SHORT_MAX;
			}
			else if (val < SHORT_MIN)
			{
				regs.SAT = 1;
				val = SHORT_MIN;
			}
			return (int16)val;
		}

		template <uint8 CTRL>
		static inline void vadduhs(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint16<0>() = vsat16(regs, (int32)a.AsInt16<0>() + (int32)b.AsInt16<0>());
			out->AsUint16<1>() = vsat16(regs, (int32)a.AsInt16<1>() + (int32)b.AsInt16<1>());
			out->AsUint16<2>() = vsat16(regs, (int32)a.AsInt16<2>() + (int32)b.AsInt16<2>());
			out->AsUint16<3>() = vsat16(regs, (int32)a.AsInt16<3>() + (int32)b.AsInt16<3>());
			out->AsUint16<4>() = vsat16(regs, (int32)a.AsInt16<4>() + (int32)b.AsInt16<4>());
			out->AsUint16<5>() = vsat16(regs, (int32)a.AsInt16<5>() + (int32)b.AsInt16<5>());
			out->AsUint16<6>() = vsat16(regs, (int32)a.AsInt16<6>() + (int32)b.AsInt16<6>());
			out->AsUint16<7>() = vsat16(regs, (int32)a.AsInt16<7>() + (int32)b.AsInt16<7>());
		}

		template <uint8 CTRL>
		static inline void vaddubm(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint8<0 >() = a.AsUint8<0 >() + b.AsUint8<0 >();
			out->AsUint8<1 >() = a.AsUint8<1 >() + b.AsUint8<1 >();
			out->AsUint8<2 >() = a.AsUint8<2 >() + b.AsUint8<2 >();
			out->AsUint8<3 >() = a.AsUint8<3 >() + b.AsUint8<3 >();
			out->AsUint8<4 >() = a.AsUint8<4 >() + b.AsUint8<4 >();
			out->AsUint8<5 >() = a.AsUint8<5 >() + b.AsUint8<5 >();
			out->AsUint8<6 >() = a.AsUint8<6 >() + b.AsUint8<6 >();
			out->AsUint8<7 >() = a.AsUint8<7 >() + b.AsUint8<7 >();
			out->AsUint8<8 >() = a.AsUint8<8 >() + b.AsUint8<8 >();
			out->AsUint8<9 >() = a.AsUint8<9 >() + b.AsUint8<9 >();
			out->AsUint8<10>() = a.AsUint8<10>() + b.AsUint8<10>();
			out->AsUint8<11>() = a.AsUint8<11>() + b.AsUint8<11>();
			out->AsUint8<12>() = a.AsUint8<12>() + b.AsUint8<12>();
			out->AsUint8<13>() = a.AsUint8<13>() + b.AsUint8<13>();
			out->AsUint8<14>() = a.AsUint8<14>() + b.AsUint8<14>();
			out->AsUint8<15>() = a.AsUint8<15>() + b.AsUint8<15>();
		}

		template <uint8 CTRL>
		static inline void vadduwm(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = a.AsUint32<0>() + b.AsUint32<0>();
			out->AsUint32<1>() = a.AsUint32<1>() + b.AsUint32<1>();
			out->AsUint32<2>() = a.AsUint32<2>() + b.AsUint32<2>();
			out->AsUint32<3>() = a.AsUint32<3>() + b.AsUint32<3>();
		}

		template <uint8 CTRL>
		static inline void vsubuwm(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsInt32<0>() = a.AsInt32<0>() - b.AsInt32<0>();
			out->AsInt32<1>() = a.AsInt32<1>() - b.AsInt32<1>();
			out->AsInt32<2>() = a.AsInt32<2>() - b.AsInt32<2>();
			out->AsInt32<3>() = a.AsInt32<3>() - b.AsInt32<3>();
		}

		template <uint8 CTRL>
		static inline void vaddshs(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsInt16<0>() = vsat16i(regs, (int32)a.AsInt16<0>() + (int32)b.AsInt16<0>());
			out->AsInt16<1>() = vsat16i(regs, (int32)a.AsInt16<1>() + (int32)b.AsInt16<1>());
			out->AsInt16<2>() = vsat16i(regs, (int32)a.AsInt16<2>() + (int32)b.AsInt16<2>());
			out->AsInt16<3>() = vsat16i(regs, (int32)a.AsInt16<3>() + (int32)b.AsInt16<3>());
			out->AsInt16<4>() = vsat16i(regs, (int32)a.AsInt16<4>() + (int32)b.AsInt16<4>());
			out->AsInt16<5>() = vsat16i(regs, (int32)a.AsInt16<5>() + (int32)b.AsInt16<5>());
			out->AsInt16<6>() = vsat16i(regs, (int32)a.AsInt16<6>() + (int32)b.AsInt16<6>());
			out->AsInt16<7>() = vsat16i(regs, (int32)a.AsInt16<7>() + (int32)b.AsInt16<7>());
		}

		static inline uint32 vsat32(CpuRegs& regs, uint64 val)
		{
			if (val > UINT_MAX)
			{
				regs.SAT = 1;
				val = UINT_MAX;
			}
			return (uint32)val;
		}

		template <uint8 CTRL>
		static inline void vadduws(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = vsat32((uint64)a.AsUint32<0>() + (uint64)b.AsUint32<0>());
			out->AsUint32<1>() = vsat32((uint64)a.AsUint32<1>() + (uint64)b.AsUint32<1>());
			out->AsUint32<2>() = vsat32((uint64)a.AsUint32<2>() + (uint64)b.AsUint32<2>());
			out->AsUint32<3>() = vsat32((uint64)a.AsUint32<3>() + (uint64)b.AsUint32<3>());
		}


		template <uint8 CTRL>
		static inline void vpkswss(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsInt16<0>() = vsat16i(regs, a.AsInt32<0>());
			out->AsInt16<1>() = vsat16i(regs, a.AsInt32<1>());
			out->AsInt16<2>() = vsat16i(regs, a.AsInt32<2>());
			out->AsInt16<3>() = vsat16i(regs, a.AsInt32<3>());
			out->AsInt16<4>() = vsat16i(regs, b.AsInt32<0>());
			out->AsInt16<5>() = vsat16i(regs, b.AsInt32<1>());
			out->AsInt16<6>() = vsat16i(regs, b.AsInt32<2>());
			out->AsInt16<7>() = vsat16i(regs, b.AsInt32<3>());
		}

		template <uint8 CTRL>
		static inline void vpkshus(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint8<0>() = vsat8i(regs, a.AsInt16<0>());
			out->AsUint8<1>() = vsat8i(regs, a.AsInt16<1>());
			out->AsUint8<2>() = vsat8i(regs, a.AsInt16<2>());
			out->AsUint8<3>() = vsat8i(regs, a.AsInt16<3>());
			out->AsUint8<4>() = vsat8i(regs, a.AsInt16<4>());
			out->AsUint8<5>() = vsat8i(regs, a.AsInt16<5>());
			out->AsUint8<6>() = vsat8i(regs, a.AsInt16<6>());
			out->AsUint8<7>() = vsat8i(regs, a.AsInt16<7>());
			out->AsUint8<8>() = vsat8i(regs, b.AsInt16<0>());
			out->AsUint8<9>() = vsat8i(regs, b.AsInt16<1>());
			out->AsUint8<10>() = vsat8i(regs, b.AsInt16<2>());
			out->AsUint8<11>() = vsat8i(regs, b.AsInt16<3>());
			out->AsUint8<12>() = vsat8i(regs, b.AsInt16<4>());
			out->AsUint8<13>() = vsat8i(regs, b.AsInt16<5>());
			out->AsUint8<14>() = vsat8i(regs, b.AsInt16<6>());
			out->AsUint8<15>() = vsat8i(regs, b.AsInt16<7>());
		}

		template <uint8 CTRL>
		static inline void vcmpbfp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			static const uint32 bit0 = 0x80000000;
			static const uint32 bit1 = 0x40000000;
			out->AsUint32<0>() = ((a.AsFloat<0>() <= b.AsFloat<0>()) ? 0 : bit0) | ((a.AsFloat<0>() >= b.AsFloat<0>()) ? 0 : bit1);
			out->AsUint32<1>() = ((a.AsFloat<1>() <= b.AsFloat<1>()) ? 0 : bit0) | ((a.AsFloat<1>() >= b.AsFloat<1>()) ? 0 : bit1);
			out->AsUint32<2>() = ((a.AsFloat<2>() <= b.AsFloat<2>()) ? 0 : bit0) | ((a.AsFloat<2>() >= b.AsFloat<2>()) ? 0 : bit1);
			out->AsUint32<3>() = ((a.AsFloat<3>() <= b.AsFloat<3>()) ? 0 : bit0) | ((a.AsFloat<3>() >= b.AsFloat<3>()) ? 0 : bit1);
		}

		template <uint8 VAL>
		static inline uint32 vspltisw_helper()
		{
			uint32 temp = (VAL & 0x1F); // 5 bits allowed
			if (temp & 0x10) // sign bit set ?
				temp |= 0xFFFFFF00; // sign extend
			return temp;
		}

		template <uint8 CTRL, uint8 SEL>
		static inline void vspltisb(CpuRegs& regs, TVReg* out)
		{
			ASM_CHECK(CTRL == 0);

			out->AsUint8<0>() = vspltisw_helper<VAL>();
			out->AsUint8<1>() = vspltisw_helper<VAL>();
			out->AsUint8<2>() = vspltisw_helper<VAL>();
			out->AsUint8<3>() = vspltisw_helper<VAL>();
			out->AsUint8<4>() = vspltisw_helper<VAL>();
			out->AsUint8<5>() = vspltisw_helper<VAL>();
			out->AsUint8<6>() = vspltisw_helper<VAL>();
			out->AsUint8<7>() = vspltisw_helper<VAL>();
			out->AsUint8<8>() = vspltisw_helper<VAL>();
			out->AsUint8<9>() = vspltisw_helper<VAL>();
			out->AsUint8<10>() = vspltisw_helper<VAL>();
			out->AsUint8<11>() = vspltisw_helper<VAL>();
			out->AsUint8<12>() = vspltisw_helper<VAL>();
			out->AsUint8<13>() = vspltisw_helper<VAL>();
			out->AsUint8<14>() = vspltisw_helper<VAL>();
			out->AsUint8<15>() = vspltisw_helper<VAL>();
		}

		template <uint8 CTRL, uint16 SEL>
		static inline void vspltish(CpuRegs& regs, TVReg* out)
		{
			ASM_CHECK(CTRL == 0);

			out->AsUint16<0>() = vspltisw_helper<VAL>();
			out->AsUint16<1>() = vspltisw_helper<VAL>();
			out->AsUint16<2>() = vspltisw_helper<VAL>();
			out->AsUint16<3>() = vspltisw_helper<VAL>();
			out->AsUint16<4>() = vspltisw_helper<VAL>();
			out->AsUint16<5>() = vspltisw_helper<VAL>();
			out->AsUint16<6>() = vspltisw_helper<VAL>();
			out->AsUint16<7>() = vspltisw_helper<VAL>();
		}

		template <uint8 CTRL, uint8 VAL>
		static inline void vspltisw(CpuRegs& regs, TVReg* out)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = vspltisw_helper<VAL>();
			out->AsUint32<1>() = vspltisw_helper<VAL>();
			out->AsUint32<2>() = vspltisw_helper<VAL>();
			out->AsUint32<3>() = vspltisw_helper<VAL>();
		}

		template <uint8 CTRL>
		static inline void vmulfp128(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = a.AsFloat<0>() * b.AsFloat<0>();
			out->AsFloat<1>() = a.AsFloat<1>() * b.AsFloat<1>();
			out->AsFloat<2>() = a.AsFloat<2>() * b.AsFloat<2>();
			out->AsFloat<3>() = a.AsFloat<3>() * b.AsFloat<3>();
		}

		template <uint8 CTRL>
		static inline void vsubfp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = a.AsFloat<0>() - b.AsFloat<0>();
			out->AsFloat<1>() = a.AsFloat<1>() - b.AsFloat<1>();
			out->AsFloat<2>() = a.AsFloat<2>() - b.AsFloat<2>();
			out->AsFloat<3>() = a.AsFloat<3>() - b.AsFloat<3>();
		}

		template <uint8 CTRL>
		static inline void vaddfp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = a.AsFloat<0>() + b.AsFloat<0>();
			out->AsFloat<1>() = a.AsFloat<1>() + b.AsFloat<1>();
			out->AsFloat<2>() = a.AsFloat<2>() + b.AsFloat<2>();
			out->AsFloat<3>() = a.AsFloat<3>() + b.AsFloat<3>();
		}

		template <uint8 CTRL>
		static inline void vrsqrtefp(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			const float r0 = sqrtf(a.AsFloat<0>());
			const float r1 = sqrtf(a.AsFloat<1>());
			const float r2 = sqrtf(a.AsFloat<2>());
			const float r3 = sqrtf(a.AsFloat<3>());
			out->AsFloat<0>() = (r0 > 0.0f) ? (1.0f / r0) : 0.0f;
			out->AsFloat<1>() = (r1 > 0.0f) ? (1.0f / r1) : 0.0f;
			out->AsFloat<2>() = (r2 > 0.0f) ? (1.0f / r2) : 0.0f;
			out->AsFloat<3>() = (r3 > 0.0f) ? (1.0f / r3) : 0.0f;
		}

		template <uint8 CTRL>
		static inline void vrfim(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = floorf(a.AsFloat<0>());
			out->AsFloat<1>() = floorf(a.AsFloat<1>());
			out->AsFloat<2>() = floorf(a.AsFloat<2>());
			out->AsFloat<3>() = floorf(a.AsFloat<3>());
		}

		template <uint8 CTRL>
		static inline void vrfin(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = (float)((a.AsFloat<0>() > 0.0f) ? (int)(a.AsFloat<0>() + 0.5f) : (int)(a.AsFloat<0>() - 0.5f));
			out->AsFloat<1>() = (float)((a.AsFloat<1>() > 0.0f) ? (int)(a.AsFloat<1>() + 0.5f) : (int)(a.AsFloat<1>() - 0.5f));
			out->AsFloat<2>() = (float)((a.AsFloat<2>() > 0.0f) ? (int)(a.AsFloat<2>() + 0.5f) : (int)(a.AsFloat<2>() - 0.5f));
			out->AsFloat<3>() = (float)((a.AsFloat<3>() > 0.0f) ? (int)(a.AsFloat<3>() + 0.5f) : (int)(a.AsFloat<3>() - 0.5f));
		}

		template <uint8 CTRL>
		static inline void vrfiz(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = (float)(int)(a.AsFloat<0>());
			out->AsFloat<1>() = (float)(int)(a.AsFloat<1>());
			out->AsFloat<2>() = (float)(int)(a.AsFloat<2>());
			out->AsFloat<3>() = (float)(int)(a.AsFloat<3>());
		}

		template <uint8 CTRL>
		static inline void vrfip(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = ceilf(a.AsFloat<0>());
			out->AsFloat<1>() = ceilf(a.AsFloat<1>());
			out->AsFloat<2>() = ceilf(a.AsFloat<2>());
			out->AsFloat<3>() = ceilf(a.AsFloat<3>());
		}

		template <uint8 CTRL>
		static inline void vmaddfp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b, const TVReg c)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = (a.AsFloat<0>() * b.AsFloat<0>()) + c.AsFloat<0>();
			out->AsFloat<1>() = (a.AsFloat<1>() * b.AsFloat<1>()) + c.AsFloat<1>();
			out->AsFloat<2>() = (a.AsFloat<2>() * b.AsFloat<2>()) + c.AsFloat<2>();
			out->AsFloat<3>() = (a.AsFloat<3>() * b.AsFloat<3>()) + c.AsFloat<3>();
		}

		template <uint8 CTRL>
		static inline void vnmsubfp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b, const TVReg c)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = -((a.AsFloat<0>() * b.AsFloat<0>()) - c.AsFloat<0>());
			out->AsFloat<1>() = -((a.AsFloat<1>() * b.AsFloat<1>()) - c.AsFloat<1>());
			out->AsFloat<2>() = -((a.AsFloat<2>() * b.AsFloat<2>()) - c.AsFloat<2>());
			out->AsFloat<3>() = -((a.AsFloat<3>() * b.AsFloat<3>()) - c.AsFloat<3>());
		}

		template <uint8 CTRL>
		static inline void vrefp(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = 1.0f / a.AsFloat<0>();
			out->AsFloat<1>() = 1.0f / a.AsFloat<1>();
			out->AsFloat<2>() = 1.0f / a.AsFloat<2>();
			out->AsFloat<3>() = 1.0f / a.AsFloat<3>();
		}

		template <uint8 CTRL>
		static inline void vslw(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = a.AsUint32<0>() << (b.AsUint32<0>() & 31);
			out->AsUint32<1>() = a.AsUint32<1>() << (b.AsUint32<1>() & 31);
			out->AsUint32<2>() = a.AsUint32<2>() << (b.AsUint32<2>() & 31);
			out->AsUint32<3>() = a.AsUint32<3>() << (b.AsUint32<3>() & 31);
		}

		template <uint8 CTRL>
		static inline void vsrw(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint32<0>() = a.AsUint32<0>() >> (b.AsUint32<0>() & 31);
			out->AsUint32<1>() = a.AsUint32<1>() >> (b.AsUint32<1>() & 31);
			out->AsUint32<2>() = a.AsUint32<2>() >> (b.AsUint32<2>() & 31);
			out->AsUint32<3>() = a.AsUint32<3>() >> (b.AsUint32<3>() & 31);
		}

		template <uint8 CTRL>
		static inline void vsrah(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint16<0>() = (uint16)((int16)a.AsUint16<0>() >> (b.AsUint16<0>() & 15));
			out->AsUint16<1>() = (uint16)((int16)a.AsUint16<1>() >> (b.AsUint16<1>() & 15));
			out->AsUint16<2>() = (uint16)((int16)a.AsUint16<2>() >> (b.AsUint16<2>() & 15));
			out->AsUint16<3>() = (uint16)((int16)a.AsUint16<3>() >> (b.AsUint16<3>() & 15));
			out->AsUint16<4>() = (uint16)((int16)a.AsUint16<4>() >> (b.AsUint16<4>() & 15));
			out->AsUint16<5>() = (uint16)((int16)a.AsUint16<5>() >> (b.AsUint16<5>() & 15));
			out->AsUint16<6>() = (uint16)((int16)a.AsUint16<6>() >> (b.AsUint16<6>() & 15));
			out->AsUint16<7>() = (uint16)((int16)a.AsUint16<7>() >> (b.AsUint16<7>() & 15));
		}

		template <uint8 CTRL>
		static inline void vsraw(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsInt32<0>() = a.AsInt32<0>() >> (b.AsUint32<0>() & 31);
			out->AsInt32<1>() = a.AsInt32<1>() >> (b.AsUint32<1>() & 31);
			out->AsInt32<2>() = a.AsInt32<2>() >> (b.AsUint32<2>() & 31);
			out->AsInt32<3>() = a.AsInt32<3>() >> (b.AsUint32<3>() & 31);
		}

		template <uint8 CTRL>
		static inline void vslo(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			uint32 shift = (b.AsUint32<0>() >> 3) & 0xF; // bytes to shift!
			for (uint32 i = 0; i < 16; ++i)
				out->u8[Reindex[i]] = (shift <= i) ? a.u8[Reindex[i + shift]] : 0;
		}

		template <uint8 CTRL>
		static inline void vsro(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			uint32 shift = (b.AsUint32<0>() >> 3) & 0xF; // bytes to shift!
			for (uint32 i = 0; i < 16; ++i)
				out->u8[Reindex[i]] = (i >= shift) ? a.u8[Reindex[i - shift]] : 0;
		}

		template <uint8 CTRL, uint8 SHIFT>
		static inline void vcfsx(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			double div = 1.0 / (double)(1 << SHIFT);
			out->AsFloat<0>() = (float)((double)a.AsInt32<0>() * div);
			out->AsFloat<1>() = (float)((double)a.AsInt32<1>() * div);
			out->AsFloat<2>() = (float)((double)a.AsInt32<2>() * div);
			out->AsFloat<3>() = (float)((double)a.AsInt32<3>() * div);
		}

		template <uint8 CTRL, uint8 SHIFT>
		static inline void vcuxwfp(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			double div = 1.0 / (double)(1 << SHIFT);
			out->AsFloat<0>() = (float)((double)a.AsUint32<0>() * div);
			out->AsFloat<1>() = (float)((double)a.AsUint32<1>() * div);
			out->AsFloat<2>() = (float)((double)a.AsUint32<2>() * div);
			out->AsFloat<3>() = (float)((double)a.AsUint32<3>() * div);
		}

		template <uint8 CTRL>
		static inline void vdot4fp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);

			float tmp = (a.AsFloat<0>() * b.AsFloat<0>());
			tmp += (a.AsFloat<1>() * b.AsFloat<1>());
			tmp += (a.AsFloat<2>() * b.AsFloat<2>());
			tmp += (a.AsFloat<3>() * b.AsFloat<3>());

			out->AsFloat<0>() = tmp;
			out->AsFloat<1>() = tmp;
			out->AsFloat<2>() = tmp;
			out->AsFloat<3>() = tmp;
		}

		template <uint8 CTRL>
		static inline void vdot3fp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);

			float tmp = (a.AsFloat<0>() * b.AsFloat<0>());
			tmp += (a.AsFloat<1>() * b.AsFloat<1>());
			tmp += (a.AsFloat<2>() * b.AsFloat<2>());

			out->AsFloat<0>() = tmp;
			out->AsFloat<1>() = tmp;
			out->AsFloat<2>() = tmp;
			out->AsFloat<3>() = tmp;
		}

		template <uint8 CTRL, uint8 MODE>
		static inline void vupkd3d128(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			ASM_CHECK(MODE == 0 || MODE == 1 || MODE == 5 || MODE == 3);

			if (MODE == 0)
			{
				const uint32 val = (a.AsUint32<3>());
				const uint32 x = (val >> 16) & 0xFF;
				const uint32 y = (val >> 8) & 0xFF;
				const uint32 z = (val >> 0) & 0xFF;
				const uint32 w = (val >> 24) & 0xFF;

				out->AsFloat<0>() = x / 255.0f;
				out->AsFloat<1>() = y / 255.0f;
				out->AsFloat<2>() = z / 255.0f;
				out->AsFloat<3>() = w / 255.0f;
			}
			else if (MODE == 1)
			{
				union
				{
					float f;
					int i;
				} x, y;

				x.f = 3.0f;
				y.f = 3.0f;

				//fprintf(stdout, "Packed=%08X,%08X,%08X,%08X\n", a.AsUint32<0>(), a.AsUint32<1>(), a.AsUint32<2>(), a.AsUint32<3>());

				x.i += (const short&)a.AsUint16<0>();
				y.i += (const short&)a.AsUint16<1>();

				out->AsFloat<0>() = x.f;
				out->AsFloat<1>() = y.f;
				out->AsFloat<2>() = 0.0f;
				out->AsFloat<3>() = 1.0f;

				//fprintf(stdout, "Unpacked=%f,%f,%f,%f\n", out->AsFloat<0>(), out->AsFloat<1>(), out->AsFloat<2>(), out->AsFloat<3>());
			}
			else if (MODE == 3)
			{
				out->AsUint32<0>() = math::FromHalf(a.AsUint16<1>());
				out->AsUint32<1>() = math::FromHalf(a.AsUint16<0>());
				out->AsUint32<2>() = 0;
				out->AsUint32<3>() = 0;
			}
			else if (MODE == 5)
			{
				out->AsUint32<0>() = math::FromHalf(a.AsUint16<1>());
				out->AsUint32<1>() = math::FromHalf(a.AsUint16<0>());
				out->AsUint32<2>() = math::FromHalf(a.AsUint16<3>());
				out->AsUint32<3>() = math::FromHalf(a.AsUint16<2>());
			}
		}

		static inline float Saturate(float x)
		{
			if (x < 0.0f) return 0.0f;
			if (x > 1.0f) return 1.0f;
			return x;
		}

		// endian conversion
		//    BE: AH AL BH BL
		//    LE: BL BH AL AH

		// u32 = merge( u32.hi, u32.lo )
		// bswap(u32) = merge( bswap(u32.lo), bswap(u32.hi) )

		// packing masks:
		//           x         y       z         w
		//           0         1       2         3
		//         0    1    2   3    4   5    6   7
		// 1  0  0x00000000_00000000_00000000_FFFFFFFF  
		// 1  1  0x00000000_00000000_FFFFFFFF_00000000  
		// 1  2  0x00000000_FFFFFFFF_00000000_00000000  
		// 1  3  0xFFFFFFFF_00000000_00000000_00000000  
		// 2  0  0x00000000_00000000_FFFFFFFF_FFFFFFFF  
		// 2  1  0x00000000_FFFFFFFF_FFFFFFFF_00000000  
		// 2  2  0xFFFFFFFF_FFFFFFFF_00000000_00000000  
		// 2  3  0xFFFFFFFF_00000000_00000000_00000000  
		// 3  0  0x00000000_00000000_FFFFFFFF_FFFFFFFF  
		// 3  1  0x00000000_FFFFFFFF_FFFFFFFF_00000000  
		// 3  2  0xFFFFFFFF_FFFFFFFF_00000000_00000000  
		// 3  3  0x00000000_00000000_00000000_FFFFFFFF  

		// VMX register (in mem)
		// | B0  | B1  | B2  | B3  | B4  | B5  | B6  | B7  | B8  | B9  | B10 | B11 | B12 | B13 | B14 | B15 |
		// |    H0     |    H1     |    H2     |    H3     |    H4     |    H5     |    H6     |    H7     |
		// |          W0           |          W1           |          W2           |          W3           |

		template <uint8 MASK, uint8 SHIFT>
		static inline void vpkd3d128_write2(TVReg* out, const uint16 a, const uint16 b)
		{
			if (MASK == 1)
			{
				if (SHIFT == 0) { out->AsUint16<7>() = a;	out->AsUint16<6>() = b; }
				else if (SHIFT == 1) { out->AsUint16<5>() = a;	out->AsUint16<4>() = b; }
				else if (SHIFT == 2) { out->AsUint16<3>() = a;	out->AsUint16<2>() = b; }
				else { out->AsUint16<1>() = a;	out->AsUint16<0>() = b; }
			}
			else if (MASK == 2)
			{
				if (SHIFT == 0) { out->AsUint16<7>() = a;	out->AsUint16<6>() = b;  out->AsUint16<5>() = 0;	out->AsUint16<4>() = 0; }
				else if (SHIFT == 1) { out->AsUint16<5>() = a;	out->AsUint16<4>() = b;  out->AsUint16<3>() = 0;	out->AsUint16<2>() = 0; }
				else if (SHIFT == 2) { out->AsUint16<3>() = a;	out->AsUint16<2>() = b;  out->AsUint16<1>() = 0;	out->AsUint16<0>() = 0; }
				else { out->AsUint16<1>() = a;	out->AsUint16<0>() = b; }
			}
			else if (MASK == 3)
			{
				if (SHIFT == 0) { out->AsUint16<7>() = a;	out->AsUint16<6>() = b;  out->AsUint16<5>() = 0;	out->AsUint16<4>() = 0; }
				else if (SHIFT == 1) { out->AsUint16<5>() = a;	out->AsUint16<4>() = b;  out->AsUint16<3>() = 0;	out->AsUint16<2>() = 0; }
				else if (SHIFT == 2) { out->AsUint16<3>() = a;	out->AsUint16<2>() = b;  out->AsUint16<1>() = 0;	out->AsUint16<0>() = 0; }
				else { out->AsUint16<7>() = 0;	out->AsUint16<6>() = 0; }
			}
		}

		template <uint8 MASK, uint8 SHIFT>
		static inline void vpkd3d128_write2(TVReg* out, const uint32 a)
		{
			if (MASK == 1)
			{
				if (SHIFT == 0) { out->AsUint32<3>() = a; }
				else if (SHIFT == 1) { out->AsUint32<2>() = a; }
				else if (SHIFT == 2) { out->AsUint32<1>() = a; }
				else { out->AsUint32<0>() = a; }
			}
			else if (MASK == 2)
			{
				if (SHIFT == 0) { out->AsUint32<3>() = a; out->AsUint32<2>() = 0; }
				else if (SHIFT == 1) { out->AsUint32<2>() = a; out->AsUint32<1>() = 0; }
				else if (SHIFT == 2) { out->AsUint32<1>() = a; out->AsUint32<0>() = 0; }
				else { out->AsUint32<0>() = a; }
			}
			else if (MASK == 3)
			{
				if (SHIFT == 0) { out->AsUint32<3>() = a; out->AsUint32<2>() = 0; }
				else if (SHIFT == 1) { out->AsUint32<2>() = a; out->AsUint32<1>() = 0; }
				else if (SHIFT == 2) { out->AsUint32<1>() = a; out->AsUint32<0>() = 0; }
				else { out->AsUint32<3>() = 0; }
			}
		}

		template <uint8 MASK, uint8 SHIFT>
		static inline void vpkd3d128_write4(TVReg* out, const uint16 a, const uint16 b, const uint16 c, const uint16 d)
		{
			if (MASK == 1)
			{
				if (SHIFT == 0) { out->AsUint16<7>() = c;	out->AsUint16<6>() = d; }
				else if (SHIFT == 1) { out->AsUint16<5>() = c;	out->AsUint16<4>() = d; }
				else if (SHIFT == 2) { out->AsUint16<3>() = c;	out->AsUint16<2>() = d; }
				else { out->AsUint16<1>() = c;	out->AsUint16<0>() = d; }
			}
			else if (MASK == 2)
			{
				if (SHIFT == 0) { out->AsUint16<7>() = c;	out->AsUint16<6>() = d;	out->AsUint16<5>() = a;	out->AsUint16<4>() = b; }
				else if (SHIFT == 1) { out->AsUint16<5>() = c;	out->AsUint16<4>() = d;	out->AsUint16<3>() = a;	out->AsUint16<2>() = b; }
				else if (SHIFT == 2) { out->AsUint16<3>() = c;	out->AsUint16<2>() = d;	out->AsUint16<1>() = a;	out->AsUint16<0>() = b; }
				else { out->AsUint16<1>() = c;	out->AsUint16<0>() = d; }
			}
			else if (MASK == 3)
			{
				if (SHIFT == 0) { out->AsUint16<7>() = c;	out->AsUint16<6>() = d;	out->AsUint16<5>() = a;	out->AsUint16<4>() = b; }
				else if (SHIFT == 1) { out->AsUint16<5>() = c;	out->AsUint16<4>() = d;	out->AsUint16<3>() = a;	out->AsUint16<2>() = b; }
				else if (SHIFT == 2) { out->AsUint16<3>() = c;	out->AsUint16<2>() = d;	out->AsUint16<1>() = a;	out->AsUint16<0>() = b; }
				else { out->AsUint16<7>() = a;	out->AsUint16<6>() = b; }
			}
		}

		template <uint8 MASK, uint8 SHIFT>
		static inline void vpkd3d128_write4(TVReg* out, const uint64 a)
		{
			const uint32 lo = (uint32)a;
			const uint32 hi = (uint32)(a >> 32);

			if (MASK == 1)
			{
				if (SHIFT == 0) { out->AsUint32<3>() = lo; }
				else if (SHIFT == 1) { out->AsUint32<2>() = lo; }
				else if (SHIFT == 2) { out->AsUint32<1>() = lo; }
				else { out->AsUint32<0>() = lo; }
			}
			else if (MASK == 2)
			{
				if (SHIFT == 0) { out->AsUint32<2>() = hi;	out->AsUint32<3>() = lo; }
				else if (SHIFT == 1) { out->AsUint32<1>() = hi;	out->AsUint32<2>() = lo; }
				else if (SHIFT == 2) { out->AsUint32<0>() = hi;	out->AsUint32<1>() = lo; }
				else { out->AsUint32<0>() = lo; }
			}
			else if (MASK == 3)
			{
				if (SHIFT == 0) { out->AsUint32<2>() = hi;	out->AsUint32<3>() = lo; }
				else if (SHIFT == 1) { out->AsUint32<1>() = hi;	out->AsUint32<2>() = lo; }
				else if (SHIFT == 2) { out->AsUint32<0>() = hi;	out->AsUint32<1>() = lo; }
				else { out->AsUint32<3>() = hi; }
			}
		}

		template <uint8 CTRL, uint8 MODE, uint8 MASK, uint8 SHIFT>
		static inline void vpkd3d128(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			ASM_CHECK(MODE >= 0 && MODE <= 6);

			if (MODE == 0) // D3DCOLOR
			{
				const uint32 x = a.AsUint32<0>() & 0xFF;
				const uint32 y = a.AsUint32<1>() & 0xFF;
				const uint32 z = a.AsUint32<2>() & 0xFF;
				const uint32 w = a.AsUint32<3>() & 0xFF;
				const uint32 ret = (w << 24) | (x << 16) | (y << 8) | (z << 0);
				vpkd3d128_write2<MASK, SHIFT>(out, ret);
			}
			else if (MODE == 1) // NORM2
			{
				const uint16 x = (uint16)(a.AsUint32<0>() & 0xFFFF);
				const uint16 y = (uint16)(a.AsUint32<1>() & 0xFFFF);
				vpkd3d128_write2<MASK, SHIFT>(out, x, y);
			}
			else if (MODE == 2) // NORMPACKED2
			{
				const uint32 ret = 0;
				vpkd3d128_write2<MASK, SHIFT>(out, ret);
			}
			else if (MODE == 3) // FLOAT16_2
			{
				const uint16 v0 = math::ToHalf(a.AsUint32<0>());
				const uint16 v1 = math::ToHalf(a.AsUint32<1>());
				vpkd3d128_write2<MASK, SHIFT>(out, v0, v1);
			}
			else if (MODE == 4) // NORM4
			{
				const uint16 x = (uint16)(a.AsUint32<0>() & 0xFFFF);
				const uint16 y = (uint16)(a.AsUint32<1>() & 0xFFFF);
				const uint16 z = (uint16)(a.AsUint32<2>() & 0xFFFF);
				const uint16 w = (uint16)(a.AsUint32<3>() & 0xFFFF);
				vpkd3d128_write4<MASK, SHIFT>(out, x, y, z, w);
			}
			else if (MODE == 5) // FLOAT16_4
			{
				const uint16 v0 = math::ToHalf(a.AsUint32<0>());
				const uint16 v1 = math::ToHalf(a.AsUint32<1>());
				const uint16 v2 = math::ToHalf(a.AsUint32<2>());
				const uint16 v3 = math::ToHalf(a.AsUint32<3>());
				vpkd3d128_write4<MASK, SHIFT>(out, v0, v1, v2, v3);
			}
			else if (MODE == 6) // NORMPACKED64
			{
				const uint64 ret = 0;
				vpkd3d128_write4<MASK, SHIFT>(out, ret);
			}
		}

		template <uint8 CTRL>
		static inline void vupkhsb(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint16<1>() = (uint16)(int16)(int8)a.u8[8 + 3];
			out->AsUint16<0>() = (uint16)(int16)(int8)a.u8[8 + 2];
			out->AsUint16<3>() = (uint16)(int16)(int8)a.u8[8 + 1];
			out->AsUint16<2>() = (uint16)(int16)(int8)a.u8[8 + 0];
			out->AsUint16<5>() = (uint16)(int16)(int8)a.u8[8 + 7];
			out->AsUint16<4>() = (uint16)(int16)(int8)a.u8[8 + 6];
			out->AsUint16<7>() = (uint16)(int16)(int8)a.u8[8 + 5];
			out->AsUint16<6>() = (uint16)(int16)(int8)a.u8[8 + 4];
		}

		template <uint8 CTRL>
		static inline void vupklsb(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsUint16<1>() = (uint16)(int16)(int8)a.u8[3];
			out->AsUint16<0>() = (uint16)(int16)(int8)a.u8[2];
			out->AsUint16<3>() = (uint16)(int16)(int8)a.u8[1];
			out->AsUint16<2>() = (uint16)(int16)(int8)a.u8[0];
			out->AsUint16<5>() = (uint16)(int16)(int8)a.u8[7];
			out->AsUint16<4>() = (uint16)(int16)(int8)a.u8[6];
			out->AsUint16<7>() = (uint16)(int16)(int8)a.u8[5];
			out->AsUint16<6>() = (uint16)(int16)(int8)a.u8[4];
		}

		template <uint8 CTRL>
		static inline void vupkhsh(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsInt32<0>() = a.AsInt16<4>();
			out->AsInt32<1>() = a.AsInt16<5>();
			out->AsInt32<2>() = a.AsInt16<6>();
			out->AsInt32<3>() = a.AsInt16<7>();
		}

		template <uint8 CTRL>
		static inline void vupklsh(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsInt32<0>() = a.AsInt16<0>();
			out->AsInt32<1>() = a.AsInt16<1>();
			out->AsInt32<2>() = a.AsInt16<2>();
			out->AsInt32<3>() = a.AsInt16<3>();
		}

		template <uint8 CTRL, uint8 SHIFT>
		static inline void vcfpsxws(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);

			const float mul = (float)(1 << SHIFT);
			out->AsInt32<0>() = (int)(a.AsFloat<0>() * mul);
			out->AsInt32<1>() = (int)(a.AsFloat<1>() * mul);
			out->AsInt32<2>() = (int)(a.AsFloat<2>() * mul);
			out->AsInt32<3>() = (int)(a.AsFloat<3>() * mul);
		}

		template <uint8 CTRL, uint8 SHIFT>
		static inline void vcfpuxws(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);

			const float mul = (float)(1 << SHIFT);
			out->AsUint32<0>() = (uint32)(a.AsFloat<0>() * mul);
			out->AsUint32<1>() = (uint32)(a.AsFloat<1>() * mul);
			out->AsUint32<2>() = (uint32)(a.AsFloat<2>() * mul);
			out->AsUint32<3>() = (uint32)(a.AsFloat<3>() * mul);
		}

		template <uint8 CTRL, uint8 SHIFT>
		static inline void vcsxwfp(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);

			const float mul = (float)(1 << SHIFT);
			out->AsFloat<0>() = (float)a.AsInt32<0>() / mul;
			out->AsFloat<1>() = (float)a.AsInt32<1>() / mul;
			out->AsFloat<2>() = (float)a.AsInt32<2>() / mul;
			out->AsFloat<3>() = (float)a.AsInt32<3>() / mul;
		}

		template <uint8 CTRL>
		static inline void vcmpgtfp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			out->AsUint32<0>() = (a.AsFloat<0>() > b.AsFloat<0>()) ? 0xFFFFFFFF : 0x00000000;
			out->AsUint32<1>() = (a.AsFloat<1>() > b.AsFloat<1>()) ? 0xFFFFFFFF : 0x00000000;
			out->AsUint32<2>() = (a.AsFloat<2>() > b.AsFloat<2>()) ? 0xFFFFFFFF : 0x00000000;
			out->AsUint32<3>() = (a.AsFloat<3>() > b.AsFloat<3>()) ? 0xFFFFFFFF : 0x00000000;

			if (CTRL == 1)
			{
				const bool allEqual = CompareHelper<4>::AllSet(out->u32, 0xFFFFFFFF);
				const bool allNotEqual = CompareHelper<4>::AllSet(out->u32, 0x0);
				regs.CR[6].so = 0;
				regs.CR[6].eq = allNotEqual ? 1 : 0;
				regs.CR[6].gt = 0;
				regs.CR[6].lt = allEqual ? 1 : 0;
			}
		}

		template <uint8 CTRL>
		static inline void vminfp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = (a.AsFloat<0>() < b.AsFloat<0>()) ? a.AsFloat<0>() : b.AsFloat<0>();
			out->AsFloat<1>() = (a.AsFloat<1>() < b.AsFloat<1>()) ? a.AsFloat<1>() : b.AsFloat<1>();
			out->AsFloat<2>() = (a.AsFloat<2>() < b.AsFloat<2>()) ? a.AsFloat<2>() : b.AsFloat<2>();
			out->AsFloat<3>() = (a.AsFloat<3>() < b.AsFloat<3>()) ? a.AsFloat<3>() : b.AsFloat<3>();
		}

		template <uint8 CTRL>
		static inline void vmaxfp(CpuRegs& regs, TVReg* out, const TVReg a, const TVReg b)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = (a.AsFloat<0>() > b.AsFloat<0>()) ? a.AsFloat<0>() : b.AsFloat<0>();
			out->AsFloat<1>() = (a.AsFloat<1>() > b.AsFloat<1>()) ? a.AsFloat<1>() : b.AsFloat<1>();
			out->AsFloat<2>() = (a.AsFloat<2>() > b.AsFloat<2>()) ? a.AsFloat<2>() : b.AsFloat<2>();
			out->AsFloat<3>() = (a.AsFloat<3>() > b.AsFloat<3>()) ? a.AsFloat<3>() : b.AsFloat<3>();
		}

		template <uint8 CTRL>
		static inline void vlogefp(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = logf(a.AsFloat<0>());
			out->AsFloat<1>() = logf(a.AsFloat<1>());
			out->AsFloat<2>() = logf(a.AsFloat<2>());
			out->AsFloat<3>() = logf(a.AsFloat<3>());
		}

		template <uint8 CTRL>
		static inline void vexptefp(CpuRegs& regs, TVReg* out, const TVReg a)
		{
			ASM_CHECK(CTRL == 0);
			out->AsFloat<0>() = expf(a.AsFloat<0>());
			out->AsFloat<1>() = expf(a.AsFloat<1>());
			out->AsFloat<2>() = expf(a.AsFloat<2>());
			out->AsFloat<3>() = expf(a.AsFloat<3>());
		}

		template <uint8 CTRL>
		static inline void lvsl(CpuRegs& regs, TVReg* out, const TReg& a, const TReg b)
		{
			const bool base = (&a == &regs.R0);
			const uint32 addr = (uint32)(base ? (b) : (a + b));
			const uint8 ptr = (addr & 0xF);

			for (int i = 0; i < 16; ++i)
			{
				const uint8 ri = (i&~3) | (3 - (i & 3));
				out->u8[ri] = i + ptr;
			}
		}

		template <uint8 CTRL>
		static inline void lvsr(CpuRegs& regs, TVReg* out, const TReg& a, const TReg b)
		{
			const bool base = (&a == &regs.R0);
			const uint32 addr = (uint32)(base ? (b) : (a + b));
			const uint8 ptr = (addr & 0xF);

			for (int i = 0; i < 16; ++i)
			{
				const uint8 ri = (i&~3) | (3 - (i & 3));
				out->u8[ri] = (16 - ptr) + i;
			}
		}

		//---------------------------------------------------------------------------------

	} // op
} // cpu