#pragma once

#include "projectMemoryView.h"

namespace tools
{

	/// memory viewer for the trace
	class TraceMemoryView : public ImageMemoryView
	{
	public:
		TraceMemoryView(const std::shared_ptr<ProjectImage>& projectImage, INavigationHelper* navigator, trace::DataFile& traceData);
		virtual ~TraceMemoryView();

		// set active trace sequence frame
		void SetTraceFrame(const TraceFrameID seq, const bool functionLocal);

	private:
		virtual bool GetAddressHitCount(const uint32 offset, char* outHitCountText) const override;

		TraceFrameID m_seq;

		TraceFrameID m_functionMinSeq;
		TraceFrameID m_functionMaxSeq;
		bool m_functionLocal;

		uint64_t m_imageBaseAddress;
		trace::DataFile* m_data;

		struct HitCountInfo
		{
			uint32_t m_prev;
			uint32_t m_next;

			inline HitCountInfo()
				: m_prev(0)
				, m_next(0)
			{}

			inline const bool empty() const
			{
				return (m_prev == 0) && (m_next == 0);
			}
		};

		typedef std::unordered_map<uint32, HitCountInfo> THitCountCache;
		mutable THitCountCache m_hitCountCache;

		void ResetHitCountCache();
		HitCountInfo BuildHitCountEntry(const uint32 offset) const;
	};

} // tools