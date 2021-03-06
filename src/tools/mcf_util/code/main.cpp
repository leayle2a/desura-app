/*
Copyright (C) 2013 Mark Chandler (Desura Net Pty Ltd)
Copyright (C) 2014 Bad Juju Games, Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.

Contact us at legal@badjuju.com.

*/

#include "Common.h"
#include "UtilFunction.h"
#include "SharedObjectLoader.h"

#define MCF_VERSION "3.1.0"

#ifdef NIX
	#include <iostream>
#endif

#include "DesuraPrintFRedirect.h"



const char* GetMcfVersion();
void InitFactory();


class ShowVersion : public UtilFunction
{
public:
	virtual uint32 getNumArgs(){return 0;}
	virtual const char* getArgDesc(size_t index){return nullptr;}

	virtual const char* getFullArg(){return "version";}
	virtual const char getShortArg(){return 'v';}
	virtual const char* getDescription(){return "Shows mcf_util version.";}

	virtual int performAction(std::vector<std::string> &args)
	{
		printf("----------------------------------------------------------------------------\n");
		printf(" MCF Utility\n");
		printf("----------------------------------------------------------------------------\n");
		printf(" Util Version: %s\n", MCF_VERSION);
		printf(" McfCore Version: %s\n", GetMcfVersion());
		printf("\n");
		printf("Copyright Linden Labs 2013 (http://github.com/desura/Desurium)\n");
		printf("----------------------------------------------------------------------------\n");
		return 0;
	}
};

REG_FUNCTION(ShowVersion);


void LogMsg(MSG_TYPE type, std::string msg, Color* col, std::map<std::string, std::string> *mpArgs)
{
	fprintf(stdout, "%s", msg.c_str());
}

bool SortFunctionList(UtilFunction* a, UtilFunction* b)
{
	return a->getShortArg() < b->getShortArg();
}

#ifdef WIN32
typedef BOOL(WINAPI* SetDllDirectoryFunc)(LPCTSTR);

bool SetDllDir(const char* dir)
{
	SharedObjectLoader sol;

	if (sol.load("kernel32.dll"))
	{
		SetDllDirectoryFunc set_dll_directory = sol.getFunction<SetDllDirectoryFunc>("SetDllDirectoryA");

		if (set_dll_directory && set_dll_directory(dir))
			return true;
	}

	return false;
}

#endif


void dispHelp()
{
	std::vector<UtilFunction*> list;
	GetFunctionList(list);

	std::sort(list.begin(), list.end(), SortFunctionList);

	printf("----------------------------------------------------------------------------\n");
	printf(" MCF Utility\n");
	printf("----------------------------------------------------------------------------\n");
	printf("\n");
	printf("Usage:\n");

	for (size_t x=0; x<list.size(); x++)
	{
		printf(" -%c, --%s ", list[x]->getShortArg(), list[x]->getFullArg());

		for (size_t y=0; y<list[x]->getNumArgs(); y++)
		{
			printf(" [%s] ", list[x]->getArgDesc(y));
		}

		printf("\n\t%s\n\n", list[x]->getDescription());
	}

	printf("\n");
	printf("Copyright Linden Labs 2013 (http://github.com/desura/Desurium)\n");
	printf("----------------------------------------------------------------------------\n");
}

int main(int argc, char** argv)
{
	int res = 0;

	if (argc < 2)
	{
		dispHelp();
		return 1;
	}

#ifdef WIN32
	if (UTIL::FS::isValidFolder("bin"))
		SetDllDir(".\\bin");
#endif


	InitFactory();

	std::vector<UtilFunction*> list;
	GetFunctionList(list);

	std::vector<std::string> args;
	UtilFunction* funct = nullptr;

	for (size_t x=2; x<(size_t)argc; x++)
	{
		if (argv[x])
			args.push_back(argv[x]);
		else
			break;
	}

	size_t len = strlen(argv[1]);

	if (len == 2 && argv[1][0] == '-')
	{
		for (size_t x=0; x<list.size(); x++)
		{
			if (list[x]->getShortArg() == argv[1][1])
			{
				funct = list[x];
				break;
			}
		}
	}

	if (len > 2 && argv[1][0] == '-' && argv[1][1] == '-')
	{
		std::string arg(argv[1]+2, len-2);

		for (size_t x=0; x<list.size(); x++)
		{
			if (arg == list[x]->getFullArg())
			{
				funct = list[x];
				break;
			}
		}
	}

	if (funct)
	{
		if (funct->getNumArgs() > args.size())
		{
			printf("Not enought arguments. Expected:");

			for (size_t y=0; y<funct->getNumArgs(); y++)
			{
				printf(" [%s] ", funct->getArgDesc(y));
			}

			printf("\n");

			res = -1;
		}
		else
		{
			printf("Running %s...\n", funct->getFullArg());

			try
			{
				res = funct->performAction(args);
				funct->checkException();
			}
			catch (gcException &e)
			{
				printf("%s Failed %s [%d.%d]\n", funct->getFullArg(), e.getErrMsg(), e.getErrId(), e.getSecErrId());
				res = (e.getErrId()&0x0000FFFF) + (e.getSecErrId()<<16);
			}
			catch (std::exception &e)
			{
				printf("%s Failed %s\n", funct->getFullArg(), e.what());
				res = -1;
			}
			catch (...)
			{
				printf("%s Failed: Failed to catch exception. :(\n", funct->getFullArg());
				res = -1;
			}
		}
	}
	else
	{
		printf("Unknown Command Line Arg: %s. (no args for help)\n", argv[1]);
	}

	printf("--- Done ---\n");

	return res;
}






















