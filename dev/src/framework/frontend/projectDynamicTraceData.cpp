#include "build.h"
#include "projectTraceData.h"
#include "projectDynamicTraceData.h"

ProjectDynamicTraceData::ProjectDynamicTraceData(const class ProjectTraceData* data)
	: m_data(data)
{
}

ProjectDynamicTraceData::~ProjectDynamicTraceData()
{
}

const ProjectDynamicTraceData* ProjectDynamicTraceData::GetDynamicTraceData(const uint32 entryIndex)
{
	return NULL;
}