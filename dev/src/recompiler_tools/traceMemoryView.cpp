#include "build.h"
#include "traceMemoryView.h"
#include "projectImage.h"

#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/image.h"
#include "../recompiler_core/traceDataFile.h"

namespace tools
{
	TraceMemoryView::TraceMemoryView(const std::shared_ptr<ProjectImage>& projectImage, INavigationHelper* navigator, trace::DataFile& traceData)
		: ImageMemoryView(projectImage, navigator)
		, m_seq(INVALID_TRACE_FRAME_ID)
		, m_data(&traceData)
		, m_functionMinSeq(INVALID_TRACE_FRAME_ID)
		, m_functionMaxSeq(INVALID_TRACE_FRAME_ID)
		, m_functionLocal(false)
	{
		m_imageBaseAddress = projectImage->GetEnvironment().GetImage()->GetBaseAddress();
	}

	TraceMemoryView::~TraceMemoryView()
	{
	}

	void TraceMemoryView::SetTraceFrame(const TraceFrameID seq, const bool functionLocal)
	{
		if (m_seq != seq)
		{
			m_seq = seq;
			m_data->GetInnerMostCallFunction(seq, m_functionMinSeq, m_functionMaxSeq);
		}

		m_functionLocal = functionLocal;
		ResetHitCountCache();
	}

	void TraceMemoryView::ResetHitCountCache()
	{
		m_hitCountCache.clear();
	}

	TraceMemoryView::HitCountInfo TraceMemoryView::BuildHitCountEntry(const uint32 offset) const
	{
		// invalid crap
		if (m_seq == INVALID_TRACE_FRAME_ID)
			return TraceMemoryView::HitCountInfo();

		// get the trace page for different code address but in the current sequence context
		const auto& frame = m_data->GetFrame(m_seq);
		const auto codeAddress = m_imageBaseAddress + offset;
		const auto* codePage = m_data->GetCodeTracePage(frame.GetLocationInfo().m_contextId, codeAddress);
		if (!codePage)
			return TraceMemoryView::HitCountInfo();

		// get bounds for the histogram search (global/local)
		TraceFrameID lowerTraceSeq = 0;
		TraceFrameID upperTraceSeq = m_data->GetNumDataFrames();;
		if (m_functionLocal)
		{
			lowerTraceSeq = m_functionMinSeq;
			upperTraceSeq = m_functionMaxSeq;
		}

		// get histogram data
		TraceMemoryView::HitCountInfo ret;
		if (!m_data->GetCodeTracePageHistogram(*codePage, m_seq, lowerTraceSeq, upperTraceSeq, codeAddress, ret.m_prev, ret.m_next))
			return TraceMemoryView::HitCountInfo();

		return ret;
	}

	bool TraceMemoryView::GetAddressHitCount(const uint32 offset, char* outHitCountText) const
	{
		HitCountInfo info;

		auto it = m_hitCountCache.find(offset);
		if (it != m_hitCountCache.end())
		{
			info = (*it).second;
		}
		else
		{
			info = BuildHitCountEntry(offset);
			m_hitCountCache[offset] = info;
		}

		if (info.empty())
			return false;

		sprintf_s(outHitCountText, 16, "%6u %6u", info.m_prev, info.m_next + info.m_prev);
		return true;
	}


} // tools
