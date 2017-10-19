#include "build.h"
#include "traceInfoView.h"
#include "widgetHelpers.h"
#include "utils.h"
#include "project.h"
#include "htmlBuilder.h"

#include "../recompiler_core/traceDataFile.h"
#include "../recompiler_core/decodingInstruction.h"
#include "../recompiler_core/decodingEnvironment.h"
#include "../recompiler_core/decodingContext.h"
#include "../recompiler_core/decodingInstructionInfo.h"

#include "../recompiler_core/platformCPU.h"

namespace tools
{

	//---------------------------------------------------------------------------

	BEGIN_EVENT_TABLE(TraceInfoView, wxPanel)
		EVT_HTML_LINK_CLICKED(XRCID("TraceInfo"), TraceInfoView::OnLinkClicked)
	END_EVENT_TABLE()

	//---------------------------------------------------------------------------

	TraceInfoView::TraceInfoView(wxWindow* parent, trace::DataFile& traceData, Project* project)
		: m_infoView(NULL)
		, m_project(project)
		, m_traceData(&traceData)
	{
		wxXmlResource::Get()->LoadPanel(this, parent, wxT("TracePanel"));

		m_infoView = XRCCTRL(*this, "TraceInfo", wxHtmlWindow);

		Layout();
		Show();
	}

	TraceInfoView::~TraceInfoView()
	{
	}

	void TraceInfoView::SetFrame(const TraceFrameID id, const trace::RegDisplayFormat format)
	{
		// get the frames
		const trace::DataFrame& frame = m_traceData->GetFrame(id);
		const trace::DataFrame& nextFrame = m_traceData->GetFrame(id+1);

		// show the content of the trace frame
		if (frame.GetType() == trace::FrameType::CpuInstruction)
		{
			// get the current address
			const auto address = frame.GetAddress();

			// decode instruction
			decoding::Instruction op;
			const auto context = m_project->GetDecodingContext(address);
			if (context)
				context->DecodeInstruction(wxTheApp->GetLogWindow(), address, op, false);

			// build document
			HTMLBuilder doc(true);
			BuildCpuInstructionDoc(doc, op, frame, nextFrame, format);
			m_infoView->SetPage(doc.Extract());
		}
	}

	class RegisterListCollector
	{
	public:
		struct RegInfo
		{
			const platform::CPURegister*		m_reg;
			bool							m_dependency;
			bool							m_output;

			inline bool operator==(const RegInfo& other) const
			{
				return m_reg == other.m_reg;
			}

			inline bool operator<(const RegInfo& other) const
			{
				return _stricmp(m_reg->GetName(), other.m_reg->GetName()) < 0;
			}
		};

		std::vector< RegInfo >		m_regs;

		void AddDependency(const platform::CPURegister* reg)
		{
			for (uint32 i = 0; i < m_regs.size(); ++i)
			{
				if (m_regs[i].m_reg == reg)
				{
					m_regs[i].m_dependency = true;
					return;
				}
			}

			if (reg->GetParent())
				AddDependency(reg->GetParent());

			RegInfo info;
			info.m_reg = reg;
			info.m_dependency = true;
			info.m_output = false;
			m_regs.push_back(info);
		}

		void AddOutput(const platform::CPURegister* reg)
		{
			for (uint32 i = 0; i < m_regs.size(); ++i)
			{
				if (m_regs[i].m_reg == reg)
				{
					m_regs[i].m_output = true;
					return;
				}
			}

			if (reg->GetParent())
				AddOutput(reg->GetParent());

			RegInfo info;
			info.m_reg = reg;
			info.m_dependency = false;
			info.m_output = true;
			m_regs.push_back(info);
		}

		void Sort()
		{
			std::sort(m_regs.begin(), m_regs.end());
		}
	};

	static const char* GetRegisterTypeName(const platform::CPURegister* reg)
	{
		switch (reg->GetType())
		{
			case platform::CPURegisterType::Control:
			case platform::CPURegisterType::Generic:
			{
				switch (reg->GetBitSize())
				{
					case 1: return "ubyte1";
					case 2: return "ubyte2";
					case 4: return "ubyte4";
					case 8: return "ubyte8";
					case 16: return "ubyte16";
					case 32: return "ubyte32";
					case 64: return "ubyte64";
					case 128: return "ubyte128";
					case 256: return "ubyte256";
				}

				return "<unknown>";
			}

			case platform::CPURegisterType::FloatingPoint:
			{
				if (reg->GetBitSize() == 64)
					return "double";
				else
					return "float"; // hack
			}

			case platform::CPURegisterType::Wide:
			{
				return "wide";
			}
		}

		return "<unknown>";
	}

	void TraceInfoView::BuildCpuInstructionDoc(class HTMLBuilder& doc, const class decoding::Instruction& op, const trace::DataFrame& frame, const trace::DataFrame& nextFrame, const trace::RegDisplayFormat format)
	{
		if (!op.IsValid())
		{
			doc.Print("Invalid instruction");
			return;
		}

		// get instruction description (raw)
		char instructionText[512];
		{
			char* writeStream = instructionText;
			op.GenerateText(frame.GetAddress(), writeStream, instructionText + sizeof(instructionText));
		}

		// general instruction header
		const auto numFrames = m_traceData->GetNumDataFrames();
		doc.Print("Instruction: <b>%s</b><br>", instructionText);
		doc.Print("Address: %06llXh<br>", frame.GetAddress());
		doc.Print("Trace: %llu/%llu (%1.3f%%)<br>", frame.GetLocationInfo().m_seq, numFrames, (double)frame.GetLocationInfo().m_seq / (double)numFrames * 100.0);
		doc.Print("<br>");

		/*// time machine
		{
			char command[128];
			sprintf_s(command, "timemachine:%u", frame.GetIndex());
			doc.Print("Open <a href=\"%s\"/>TIME MACHINE</a><br>", command);
		}*/

		// get extra information
		decoding::InstructionExtendedInfo info;
		decoding::Context* context = m_project->GetDecodingContext(frame.GetAddress());
		if (op.GetExtendedInfo(frame.GetAddress(), *context, info))
		{
			// branch info
			wxString codeFlags = "";
			if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Nop) codeFlags += "Nop ";
			if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Jump) codeFlags += "Jump ";
			if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Call) codeFlags += "Call ";
			if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Return) codeFlags += "Return ";
			if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Diverge) codeFlags += "Diverge ";
			if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Conditional) codeFlags += "Conditional ";
			if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_ModifyFlags) codeFlags += "Flags ";
			if (info.m_codeFlags & decoding::InstructionExtendedInfo::eInstructionFlag_Static) codeFlags += "Static ";

			if (codeFlags.empty())
				doc.Print("Instruction flags: <i>none</i><br>");
			else
				doc.Print("Instruction flags: <b>%s</b><br>", codeFlags.c_str().AsChar());

			// branch target
			if (info.m_codeFlags & (decoding::InstructionExtendedInfo::eInstructionFlag_Jump | decoding::InstructionExtendedInfo::eInstructionFlag_Call | decoding::InstructionExtendedInfo::eInstructionFlag_Return))
			{
				uint64 branchTargetAddres = 0;
				if (info.ComputeBranchTargetAddress(frame, branchTargetAddres))
				{
					doc.Print("Branch target: <b>%08Xh %s</b><br>",
						branchTargetAddres,
						(nextFrame.GetAddress() == branchTargetAddres) ?
						"<font color=#005500>(taken)</font>" :
						"<font color=#550000>(not taken)</font>");
				}
				else
				{
					doc.Print("Branch target: <i>unknown</i><br>");
				}
			}

			doc.Print("<br>");

			// registers ?
			if (info.m_registersModifiedCount + info.m_registersDependenciesCount > 0)
			{
				doc.Open("table");
				doc.Attr("border", "1");
				doc.Attr("bordercolor", "#000000");

				// collect registers
				RegisterListCollector regs;
				for (uint32 i = 0; i < info.m_registersDependenciesCount; ++i)
					regs.AddDependency(info.m_registersDependencies[i]);
				for (uint32 i = 0; i < info.m_registersModifiedCount; ++i)
					regs.AddOutput(info.m_registersModified[i]);

				// sort registers by name
				regs.Sort();

				// header
				{
					doc.Open("tr");
					doc.Attr("bgcolor", "#AAAAAA");
					{
						doc.Open("th");
						doc.Attr("width", "20");
						doc.Print("R");
						doc.Close();
					}
					{
						doc.Open("th");
						doc.Attr("width", "20");
						doc.Print("W");
						doc.Close();
					}
					{
						doc.Open("th");
						doc.Attr("width", "70");
						doc.Print("Register");
						doc.Close();
					}
					{
						doc.Open("th");
						doc.Attr("width", "70");
						doc.Print("Type");
						doc.Close();
					}
					{
						doc.Open("th");
						doc.Attr("width", "210");
						doc.Print("Input");
						doc.Close();
					}
					{
						doc.Open("th");
						doc.Attr("width", "210");
						doc.Print("Output");
						doc.Close();
					}
					/*{
						doc.Open("th");
						doc.Attr("width", "60");
						doc.Print("Access");
						doc.Close();
					}*/
					doc.Close(); // tr
				}

				// display register table
				for (uint32 i = 0; i < regs.m_regs.size(); ++i)
				{
					const RegisterListCollector::RegInfo& info = regs.m_regs[i];

					doc.Open("tr");
					doc.PrintBlock("td", info.m_dependency ? "*" : "");
					doc.PrintBlock("td", info.m_output ? "*" : "");
					doc.PrintBlock("td", "%s", info.m_reg->GetName());
					doc.PrintBlock("td", "%s", GetRegisterTypeName(info.m_reg));

					// reg value
					{
						const auto thisVal = trace::GetRegisterValueText(info.m_reg, frame, format);
						doc.PrintBlock("td", "%s", thisVal.c_str());

						const auto nextVal = trace::GetRegisterValueText(info.m_reg, nextFrame, format);
						doc.PrintBlock("td", "%s", nextVal.c_str());
					}

					// reg scan
					/*{
						if (info.m_dependency && info.m_output)
						{
							doc.PrintBlock("td", "<a href=\"regscan:%d,2,-1,%s\">W</a> <a href=\"regscan:%d,1,1,%s\">R</a>",
								frame.GetIndex(), info.m_reg->GetName(),
								frame.GetIndex(), info.m_reg->GetName());
						}
						else if (info.m_dependency)
						{
							doc.PrintBlock("td", "<a href=\"regscan:%d,2,-1,%s\">W</a>",
								frame.GetIndex(), info.m_reg->GetName());
						}
						else if (info.m_output)
						{
							doc.PrintBlock("td", "<a href=\"regscan:%d,1,1,%s\">R</a>",
								frame.GetIndex(), info.m_reg->GetName());
						}
						else
						{
							doc.PrintBlock("td", " ");
						}
					}*/

					doc.Close();
				}

				doc.Close();
				doc.Print("<br>");
				doc.Print("<br>");
			}

			// trace history
			/*if (info.m_registersDependenciesCount)
			{
				doc.Print("<br>");
				doc.Print("Dump reg history for %08Xh:<br>", frame.GetAddress());

				for (uint32 i = 0; i < info.m_registersDependenciesCount; ++i)
				{
					const platform::CPURegister* reg = info.m_registersDependencies[i];
					{
						doc.Print("In register %s: ", reg->GetName());
						doc.Print("<a href=\"history:%0Xh,0,%d,%s\">PREV</a> ", frame.GetAddress(), frame.GetIndex(), reg->GetName());
						doc.Print("<a href=\"history:%0Xh,%d,-1,%s\">NEXT</a> ", frame.GetAddress(), frame.GetIndex(), reg->GetName());
						doc.Print("<a href=\"history:%0Xh,0,-1,%s\">ALL</a> ", frame.GetAddress(), reg->GetName());
						doc.Print("<br>");
					}
				}
				for (uint32 i = 0; i < info.m_registersModifiedCount; ++i)
				{
					const platform::CPURegister* reg = info.m_registersModified[i];
					{
						doc.Print("Out register %s: ", reg->GetName());
						doc.Print("<a href=\"history:%0Xh,0,%d,%s\">PREV</a> ", frame.GetAddress() + 4, frame.GetIndex(), reg->GetName());
						doc.Print("<a href=\"history:%0Xh,%d,-1,%s\">NEXT</a> ", frame.GetAddress() + 4, frame.GetIndex(), reg->GetName());
						doc.Print("<a href=\"history:%0Xh,0,-1,%s\">ALL</a> ", frame.GetAddress() + 4, reg->GetName());
						doc.Print("<br>");
					}
				}


				doc.Print("<br>");
			}*/

			// memory access
			if (info.m_memoryFlags & (decoding::InstructionExtendedInfo::eMemoryFlags_Read | decoding::InstructionExtendedInfo::eMemoryFlags_Write | decoding::InstructionExtendedInfo::eMemoryFlags_Touch))
			{
				// initial stuff
				const char* readFlag = (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Read) ? "Read " : "";
				const char* writeFlag = (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Write) ? "Write " : "";
				const char* touchFlag = (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Touch) ? "Touch " : "";
				const char* alignFlag = (info.m_memoryFlags & decoding::InstructionExtendedInfo::eMemoryFlags_Aligned) ? "(aligned)" : "";
				doc.Print("Memory access: <b>%s%s%s</b><br>", readFlag, writeFlag, touchFlag);
				doc.Print("Memory size: <b>%d%s</b><br>", info.m_memorySize, alignFlag);

				// address expression
				if (info.m_memoryAddressBase && info.m_memoryAddressIndex)
				{
					doc.Print("Address expr: <b>%s + %d*%s + %d(%08Xh)</b><br>",
						info.m_memoryAddressBase->GetName(),
						info.m_memoryAddressScale,
						info.m_memoryAddressIndex->GetName(),
						info.m_memoryAddressOffset,
						info.m_memoryAddressOffset);
				}
				else if (info.m_memoryAddressBase)
				{
					doc.Print("Address expr: <b>%s + %d(%08Xh)</b><br>",
						info.m_memoryAddressBase->GetName(),
						info.m_memoryAddressOffset,
						info.m_memoryAddressOffset);
				}
				else if (info.m_memoryAddressBase)
				{
					doc.Print("Address expr: <b>%d(%08Xh)</b><br>",
						info.m_memoryAddressBase->GetName(),
						info.m_memoryAddressOffset,
						info.m_memoryAddressOffset);
				}

				// compute memory address
				uint64 memoryAddress = 0;
				if (info.ComputeMemoryAddress(frame, memoryAddress))
				{
					doc.Print("Address value: <b>%08llXh</b><br>", memoryAddress);
				}
				else
				{
					doc.Print("Address value: <i>invalid</i><br>", memoryAddress);
				}

				doc.Print("<br>");

				// TEMPSHIT: show memory history
				/*{
					std::vector<ProjectTraceMemoryHistoryInfo> memHistory;
					//if (wxTheFrame->GetProject()->GetTrace()->GetMemoryHistory(memoryAddress, info.m_memorySize, memHistory))
					{
						doc.Open("table");
						doc.Attr("border", "1");
						doc.Attr("bordercolor", "#000000");

						doc.Open("tr");
						{
							doc.Open("th");
							doc.Attr("width", "100");
							doc.Print("Entry");
							doc.Close();
						}
						{
							doc.Open("th");
							doc.Attr("width", "400");
							doc.Print("Data");
							doc.Close();
						}
						doc.Close(); // tr

						for (uint32 i = 0; i < memHistory.size(); ++i)
						{
							const ProjectTraceMemoryHistoryInfo& memInfo = memHistory[i];

							doc.Open("tr");
							{
								// trace entry
								doc.PrintBlock("td", "<a href=\"entry:%d\">%d</a>", memInfo.m_entry, memInfo.m_entry);

								// memory values
								doc.Open("td");
								for (uint32 j = 0; j < info.m_memorySize; ++j)
								{
									const uint16 mask = (uint16)(1 << j);
									const uint8 data = memInfo.m_data[j];

									if (memInfo.m_validMask & mask)
									{
										if (memInfo.m_opMask & mask) // write
											doc.Print("<font color=\"#990000\">%02X </font>", data); // write
										else
											doc.Print("<font color=\"#009900\">%02X </font>", data); // read
									}
									else
									{
										doc.Print("<font color=\"#999999\">xx </font>", data); // nothing
									}
								}
								doc.Close(); //td
							}
							doc.Close();
						}

						doc.Close(); // table
					}
				}*/
			}
		}
	}

	void TraceInfoView::OnLinkClicked(wxHtmlLinkEvent& link)
	{
		const wxString href = link.GetLinkInfo().GetHref();
		//wxTheFrame->NavigateUrl(href);
	}

	//---------------------------------------------------------------------------

} // tools