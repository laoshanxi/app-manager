#pragma once

#include <boost/filesystem.hpp>
#include <chrono>
#include <list>
#include <memory>
#include <numeric>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/types.h> // For pid_t.

#include "../../common/Utility.h"

namespace os
{

	struct Process
	{
		Process(pid_t _pid,
				pid_t _parent,
				pid_t _group,
				const pid_t &_session,
				const uint64_t &_rss_bytes,
				const unsigned long &_utime,
				const unsigned long &_stime,
				const unsigned long &_cutime,
				const unsigned long &_cstime,
				const std::string &_command,
				bool _zombie)
			: pid(_pid),
			  parent(_parent),
			  group(_group),
			  session(_session),
			  rss_bytes(_rss_bytes),
			  utime(_utime),
			  stime(_stime),
			  cutime(_cutime),
			  cstime(_cstime),
			  command(_command),
			  zombie(_zombie) {}

		const pid_t pid;
		const pid_t parent;
		const pid_t group;
		const pid_t session;
		// Resident Set Size
		const uint64_t rss_bytes;
		const unsigned long utime;
		const unsigned long stime;
		const unsigned long cutime;
		const unsigned long cstime;
		const std::string command;
		const bool zombie;

		bool operator<(const Process &p) const { return pid < p.pid; }
		bool operator<=(const Process &p) const { return pid <= p.pid; }
		bool operator>(const Process &p) const { return pid > p.pid; }
		bool operator>=(const Process &p) const { return pid >= p.pid; }
		bool operator==(const Process &p) const { return pid == p.pid; }
		bool operator!=(const Process &p) const { return pid != p.pid; }
	};

	class ProcessTree
	{
	public:
		// Returns a process subtree rooted at the specified PID, or none if
		// the specified pid could not be found in this process tree.
		std::shared_ptr<ProcessTree> find(pid_t pid) const
		{
			if (process.pid == pid)
			{
				// make a copy of this
				return std::make_shared<ProcessTree>(*this);
			}

			for (const ProcessTree &tree : children)
			{
				std::shared_ptr<ProcessTree> option = tree.find(pid);
				if (option != nullptr)
				{
					return option;
				}
			}

			return nullptr;
		}

		// Count the total RES memory usage in the process tree
		uint64_t totalRssMemBytes() const
		{
			uint64_t result = std::accumulate(children.begin(), children.end(), process.rss_bytes);
			return result;
		}

		// get total CPU time
		uint64_t totalCpuTime() const
		{
			auto result = std::accumulate(children.begin(), children.end(), process.utime) +
						  std::accumulate(children.begin(), children.end(), process.stime) +
						  std::accumulate(children.begin(), children.end(), process.cutime) +
						  std::accumulate(children.begin(), children.end(), process.cstime);
			return result;
		}

		std::list<os::Process> getProcesses() const
		{
			std::list<os::Process> result;
			result.push_back(this->process);
			for (auto tree : children)
			{
				result.merge(tree.getProcesses());
			}
			return result;
		}

		// Checks if the specified pid is contained in this process tree.
		bool contains(pid_t pid) const
		{
			return find(pid) != nullptr;
		}

		operator Process() const
		{
			return process;
		}

		operator pid_t() const
		{
			return process.pid;
		}

		const Process process;
		const std::list<ProcessTree> children;

	private:
		friend std::shared_ptr<ProcessTree> pstree(pid_t, const std::list<Process> &);

		ProcessTree(
			const Process &_process,
			const std::list<ProcessTree> &_children)
			: process(_process),
			  children(_children) {}
	};

	inline std::ostream &operator<<(std::ostream &stream, const ProcessTree &tree)
	{
		if (tree.children.empty())
		{
			stream << "--- " << tree.process.pid << " ";
			if (tree.process.zombie)
			{
				stream << "(" << tree.process.command << ")";
			}
			else
			{
				stream << tree.process.command;
			}
		}
		else
		{
			stream << "-+- " << tree.process.pid << " ";
			if (tree.process.zombie)
			{
				stream << "(" << tree.process.command << ")";
			}
			else
			{
				stream << tree.process.command;
			}
			size_t size = tree.children.size();
			for (const ProcessTree &child : tree.children)
			{
				std::ostringstream out;
				out << child;
				stream << "\n";
				if (--size != 0)
				{
					stream << " |" << Utility::stringReplace(out.str(), "\n", "\n |");
				}
				else
				{
					stream << " \\" << Utility::stringReplace(out.str(), "\n", "\n  ");
				}
			}
		}
		return stream;
	}

	inline std::ostream &operator<<(std::ostream &stream, const std::list<os::ProcessTree> &list)
	{
		stream << "[ " << std::endl;
		std::list<os::ProcessTree>::const_iterator iterator = list.begin();
		while (iterator != list.end())
		{
			stream << *iterator;
			if (++iterator != list.end())
			{
				stream << std::endl
					   << std::endl;
			}
		}
		stream << std::endl
			   << "]";
		return stream;
	}

	// https://stackoverflow.com/questions/6583158/finding-open-file-descriptors-for-a-process-linux-c-code
	// https://stackoverflow.com/questions/4470121/how-to-use-lsoflist-opened-files-in-a-c-c-application
	// get process open file descriptors
	inline size_t fileDescriptors(pid_t pid = ::getpid())
	{
		size_t result = 0;
		// 1. /proc/pid/fd/
		std::string path = "/proc/" + std::to_string(pid) + "/fd/";
		if (boost::filesystem::exists(path))
		{
			result += std::distance(boost::filesystem::directory_iterator(path),
									boost::filesystem::directory_iterator());
		}
		// 2. /proc/pid/maps
		path = "/proc/" + std::to_string(pid) + "maps";
		std::ifstream maps(path);
		if (maps.is_open())
		{
			std::string line;
			for (; std::getline(maps, line); result++)
				;
		}
		return result;
	};

} // namespace os
