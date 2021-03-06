#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
//#include <map>
#include <set>
#include <algorithm>
//#include <iomanip>

#include "gui.h"
//#include "worktune.h"


Datablock_Field::Datablock_Field(mdCommand *command_): command(command_), dataString(""),
	printString((command->mdCmdType == MD_WORD) ? "..." : ".."), arg1(""), arg2(""), modifier("") {}

Datablock_Field::Datablock_Field(const Datablock_Field &field): command(field.command), dataString(field.dataString),
	printString(field.printString), arg1(field.arg1), arg2(field.arg2), modifier(field.modifier) {}

Datablock_Field& Datablock_Field::operator=(const Datablock_Field &field) {

    command = field.command;
    dataString = field.dataString;
	printString = field.printString;
	arg1 = field.arg1;
	arg2 = field.arg2;
	modifier = field.modifier;
	return *this;
}

Datablock_Field::~Datablock_Field() {}

void Datablock_Field::set(const string &dataString_, const bool &hexMode) {

	if (dataString_ == "") {

		dataString = "";
		arg1 = "";
		arg2 = "";
		modifier = "";
		printString = (command->mdCmdType == MD_WORD) ? "..." : "..";
		return;
	}

	//number strings are handled decimal internally, automatically convert hex number strings
	size_t pos = dataString_.find_first_of("+-*/|^&");
	string arg1print = arg1;
	string arg2print = arg2;

	if (pos == string::npos) {

		if (isNumber(dataString_)) {

			if (getType(dataString_) == MD_DEC) arg1 = dataString_;
			else {

				ostringstream argstr;
				argstr << stol(trimChars(dataString_, "$"), nullptr, 16);
				arg1 = argstr.str();
			}

			arg1print = numToStr(stol(arg1, nullptr, 10), (command->mdCmdType == MD_WORD) ? 4 : 2, hexMode);
		}
		else {
			arg1 = dataString_;
			arg1print = arg1;
		}
	}
	else {

		if (!command->allowModifiers) throw ("Use of modifier not allowed on command " + command->mdCmdName);	//TODO possibly redundant

		string argstr1 = dataString_.substr(0, pos);
		modifier = dataString_.substr(pos, 1);
		string argstr2 = dataString_.substr(pos + 1);


		if (argstr1 != "") {
			if (isNumber(argstr1)) {

				if (getType(argstr1) == MD_DEC) arg1 = argstr1;
				else {

					ostringstream argstr;
					argstr << stol(trimChars(argstr1, "$"), nullptr, 16);
					arg1 = argstr.str();
				}

				arg1print = numToStr(stol(arg1, nullptr, 10), (command->mdCmdType == MD_WORD) ? 4 : 2, hexMode);
			}
			else {
				arg1 = argstr1;
				arg1print = arg1;
			}
		}

		if (isNumber(argstr2)) {

			if (getType(argstr2) == MD_DEC) arg2 = argstr2;
			else {
				ostringstream argstr;
				argstr << stol(trimChars(argstr2, "$"), nullptr, 16);
				arg2 = argstr.str();
			}

			arg2print = numToStr(stol(arg2, nullptr, 10), (command->mdCmdType == MD_WORD) ? 4 : 2, hexMode);
		}
		else {
			arg2 = argstr2;
			arg2print = arg2;
		}
	}

	//TODO: validate note range!
	if (command->useNoteNames) {

		if (arg1print == "rest") arg1print = "<=>";
		else if (arg1print == "noise") arg1print = "[\\]";
		else if (isNumber(arg1print)) {		//TODO: potential error if arg2 != 0

			if (arg1 == "0") {
				arg1print = "<=>"; //TODO temp fix
				arg1 = "rest";
			}
			else {
				modifier = "+";
				arg2 = arg1;
				arg1 = "0";
				arg1print = "  0";
				arg2print = numToStr(stol(arg2, nullptr, 10), (command->mdCmdType == MD_WORD) ? 4 : 2, hexMode);
			}
		}
		else if (arg1print.compare(1, 2, "is") == 0) arg1print = arg1print.substr(0, 1) + "#" + arg1print.substr(3, string::npos);
		else arg1print = arg1print.substr(0, 1) + "-" + arg1print.substr(1, string::npos);

		if (isNumber(arg2print)) arg2print = numToStr(stol(arg2, nullptr, 10), (command->mdCmdType == MD_WORD) ? 4 : 2, hexMode);
	}

	//TODO: MDAL will currently not accept parameters in form of "xMODy"
	if (modifier != "" && arg1 == "0" && arg2 == "0") dataString = "0";	//TODO
	else if (modifier != "" && arg1 == "0") dataString = arg2;		//TODO

	else dataString = arg1 + modifier + arg2;
	printString = arg1print + modifier + arg2print;
}

void Datablock_Field::remove_modifier() {

	if (arg2 == "") return;
	modifier = "";
	arg2 = "";
	dataString = arg1;
	printString = printString.substr(0, printString.find_last_of("+-*/^&|"));
}



Datablock_Column::Datablock_Column(mdCommand *command_): command(command_) {

	width = ((command->mdCmdType == MD_WORD) && (command->allowModifiers || command->isBlkReference)) ? 90.0f : 50.0f;
}

Datablock_Column::Datablock_Column(const Datablock_Column &col):
    command(col.command), width(col.width), columnData(col.columnData) {}


Datablock_Column& Datablock_Column::operator=(const Datablock_Column &col) {

    command = col.command;
    width = col.width;
    columnData = col.columnData;
	return *this;
}

Datablock_Column::~Datablock_Column() {}



Datablock::Datablock(const string &title): name(title) {}

Datablock::Datablock(const Datablock &blk): name(blk.name), columns(blk.columns) {}

Datablock& Datablock::operator=(const Datablock &blk) {

    name = blk.name;
    columns = blk.columns;
	return *this;
}

Datablock::~Datablock() {}


Datablock_Type::Datablock_Type() {}

Datablock_Type::Datablock_Type(const Datablock_Type &blkType): blocks(blkType.blocks), commands(blkType.commands) {}

Datablock_Type& Datablock_Type::operator=(const Datablock_Type &blkType) {

    blocks = blkType.blocks;
    commands = blkType.commands;
    return *this;
}

Datablock_Type::~Datablock_Type() {}


Work_Tune::Work_Tune(): infilePath(""), savefilePath("") {

	engineCode = nullptr;
	engineCodeNoLoop = nullptr;
	hasUnsavedChanges = false;
	engineSize = 0;
	engineSizeNoLoop = 0;
	sequenceLoopPoint = 0;
	loopPatchAddr = 0;
	orgAddress = 0;
    engineInitAddress = 0;
    engineExitAddress = 0;
    engineReloadAddress = 0;
//    firstDividerIndex = 0;
//	lastDividerIndex = 0;
	uniqueSeed = 0;
	clipboardStatus = INACTIVE;
	status = nullptr;
}

Work_Tune::~Work_Tune() {

	delete[] engineCode;
	engineCode = nullptr;
	delete[] engineCodeNoLoop;
	engineCodeNoLoop = nullptr;
}

void Work_Tune::init(const string &infile, const Global_Settings &settings, Display_Status *status_) {

    reset();
    status = status_;

	infilePath = infile;

	if (infilePath != "") {

		savefilePath = infilePath;

		string tempstr;
		bool blockComment = false;

		ifstream MDFILE(infilePath.data());
		if (!MDFILE.is_open()) throw ("Could not read " + infilePath + ".");

		while (getline(MDFILE, tempstr)) {

			string remains = "";

			if (tempstr.find("/*", 0, 2) != string::npos) {				//detect and strip block comments

				remains = tempstr.erase(tempstr.find("/*", 0, 2), tempstr.find("*/", 0, 2));
				blockComment = true;
			}

			if (tempstr.find("*/", 0, 2) != string::npos) {

				blockComment = false;
				tempstr.erase(0, tempstr.find("*/", 0, 2) + 2);
			}

			if (blockComment) tempstr = "" + remains;
			else {

				size_t commentPos = tempstr.find("//", 0, 2);			//strip regular comments
				if (commentPos != string::npos) tempstr.erase(commentPos);
			}

			//TODO why do we need to strip tabs here? should be automatically stripped by mdal
			moduleLines.push_back(trimChars(tempstr, "\t"));
		}

		configName = getArgument(string("CONFIG"), moduleLines);
	}
	else {

		configName = settings.defaultConfig;
	}

	configPath = "engines/config/" + configName + ".mdconf";

	bool verbose = false;
	config.init(configPath, verbose);

	if (config.targetPlatform == "zxspectrum48") orgAddress = SPECTRUM48_ENTRY_POINT;

	for (int i = 0; i < config.mdCmdCount; i++) {

		if (config.mdCmdList[i].mdCmdGlobalConst) globalConstants.emplace_back(Datablock_Field(&config.mdCmdList[i]));
	}

	for (auto&& it: config.blockTypes) {

		vector<bool> cmdsUsed;

		for (int i = 0; i < config.mdCmdCount; i++) cmdsUsed.push_back(false);

		for (int i = 0; i < it.blkFieldCount; i++) {

			for (int j = 0; j < config.mdCmdCount; j++) {

				if (it.blkFieldList[i].useCmd[j]) cmdsUsed[static_cast<unsigned>(j)] = true;
			}
		}


		Datablock_Type bt;

		for (int i = 0; i < config.mdCmdCount; i++) {

			if (cmdsUsed[static_cast<unsigned>(i)]) bt.commands.push_back(&config.mdCmdList[i]);
		}

		blockTypes.push_back(bt);
	}

	musicdataBinary.init("engines/" + configName + "/equates.h");

	//check if loop point must be patched in player code, and set patch address accordingly
	if (musicdataBinary.equates.count("loop_point_patch")) {
		loopPatchAddr = musicdataBinary.equates["loop_point_patch"] - orgAddress;
	}
	else loopPatchAddr = -1;

	if (musicdataBinary.equates.count("breakpoint_init")) engineInitAddress = musicdataBinary.equates["breakpoint_init"];
	else throw (string("Missing breakpoint_init declaration in equates.h"));
	if (musicdataBinary.equates.count("breakpoint_exit")) engineExitAddress = musicdataBinary.equates["breakpoint_exit"];
	else throw (string("Missing breakpoint_exit declaration in equates.h"));
	if (musicdataBinary.equates.count("breakpoint_reload")) engineReloadAddress = musicdataBinary.equates["breakpoint_reload"];
	else throw (string("Missing breakpoint_reload declaration in equates.h"));

	ifstream ENGINEBIN("engines/" + configName + "/main.bin", ios::binary);
	if (!ENGINEBIN.is_open()) throw ("Could not read engines/" + configName + "/main.bin");

	ENGINEBIN.seekg(0, ios::end);
	engineSize = static_cast<int>(ENGINEBIN.tellg());
	ENGINEBIN.seekg(0, ios::beg);
	engineCode = new char[engineSize];
	ENGINEBIN.read(engineCode, engineSize);
	ENGINEBIN.close();

	ifstream ENGINEBIN_NL("engines/" + configName + "/main_no_loop.bin", ios::binary);
	if (!ENGINEBIN_NL.is_open()) throw ("Could not read engines/" + configName + "/main_no_loop.bin");

	ENGINEBIN_NL.seekg(0, ios::end);
	engineSizeNoLoop = static_cast<int>(ENGINEBIN_NL.tellg());
	ENGINEBIN_NL.seekg(0, ios::beg);
	engineCodeNoLoop = new char[engineSizeNoLoop];
	ENGINEBIN_NL.read(engineCodeNoLoop, engineSize);
	ENGINEBIN_NL.close();

	//generate frequency divider table
	for (char noct = '0'; noct < '9'; noct++) {

        for (int n = 0; n < 12; n++) {

            string nname = noteNames[n] + noct;
            if (musicdataBinary.equates.count(nname)) freqDividers.push_back(musicdataBinary.equates[nname]);
            else freqDividers.push_back(0);
        }
	}

//	for (auto&& it: freqDividers) cout << it << endl;

	if (infilePath == "") generate_empty_module(settings.defaultBlockLength);
	mdModule mod(moduleLines, config, verbose);

	for (auto&& it: globalConstants) it.set(it.command->getValueString(), settings.hexMode);

	//TODO: add multi-track sequence support
	if (config.trackSources.size() > 1) throw (string("Multi-track sequences are currently not supported in bintracker"));
	sequenceLoopPoint = mod.seq.mdSequenceLoopPosition;
	for (auto&& it: mod.seq.sequenceData[0]) sequence.push_back(it);

	//TODO: should perhaps reserve a large chunk of memory for the vectors ahead of time
	unsigned btref = 0;

	for (auto&& bt: mod.moduleBlocks) {

		for (auto&& blk: bt.blocks) {

			Datablock block(blk.blkName);

			for (auto&& cmd: blockTypes[btref].commands) block.columns.push_back(Datablock_Column(cmd));

			vector<string> rawBlockData;

			unsigned blockBegin = 0;
			for (; (blockBegin < moduleLines.size()) && (moduleLines[blockBegin] != (":" + block.name)); blockBegin++) {};
			blockBegin++;

			unsigned blockEnd = blockBegin + 1;
			for (; (blockEnd < moduleLines.size()) && (moduleLines[blockEnd].find(":") == string::npos); blockEnd++) {};
//			blockEnd--;

			for (unsigned i = blockBegin; i < blockEnd; i++) {

				if (moduleLines[i] != "") rawBlockData.push_back(trimChars(moduleLines[i], "\t "));
			}

			for (auto&& col: block.columns) {

				for (auto&& row: rawBlockData) {

					if (col.columnData.size() >= DISPLAY_MAX_ROWS) throw (string("Block too large to display"));

					string cmdName;
					Datablock_Field field(col.command);

					if (col.command->mdCmdAuto) {

						cmdName = col.command->mdCmdName;
						if (row.find(cmdName) != string::npos) field.set("set", settings.hexMode);
					}
					else {

						cmdName = col.command->mdCmdName + "=";
						size_t pos = row.find(cmdName);

						if ((pos != string::npos) && (pos != 0)) {

							pos = row.find("," + cmdName);
							if (pos != string::npos) pos++;
						}

						if (pos != string::npos) {

							string fstr = row.substr(pos + cmdName.size(), string::npos);
							if (fstr.find_first_of(",") != string::npos)
								fstr = fstr.substr(0, fstr.find_first_of(","));

							field.set(fstr, settings.hexMode);
						}
					}

					col.columnData.push_back(field);
				}
			}

			blockTypes[btref].blocks.push_back(block);
		}

		btref++;
	}
}

void Work_Tune::reset() {

	delete[] engineCode;
	engineCode = nullptr;
	delete[] engineCodeNoLoop;
	engineCodeNoLoop = nullptr;
	orgAddress = 0;
	engineInitAddress = 0;
	engineExitAddress = 0;
    engineReloadAddress = 0;
    savefilePath = "";
    hasUnsavedChanges = false;

//	config.reset();

	globalConstants.clear();
	moduleLines.clear();
	patternLines.clear();
	singleLine.clear();
	sequence.clear();
	blockTypes.clear();
	lastUsed.clear();

	freqDividers.clear();
//	firstDividerIndex = 0;
//	lastDividerIndex = 0;

	uniqueSeed = 0;
	clipboardStatus = INACTIVE;
	clipboard.clear();
	clipboardSequence.clear();

	status = nullptr;
}

void Work_Tune::replace_reference(const string &original, const string &replacement, const string &blocktypeID) {

	for (auto&& bt: blockTypes) {
		for (auto&& blk: bt.blocks) {
			for (auto&& col: blk.columns) {
				for (auto&& field: col.columnData) {
					//TODO must also make sure that field->command.referenceBlkID == blocktypeID, disabled for now
					//until numeric references are available in MDAL
					if (field.command->isBlkReference && field.dataString == original) field.set(replacement, true);
				}
			}
		}
	}

	//TODO also replace on globals
}


void Work_Tune::update_last_used(const unsigned &block, const unsigned &row) {

	lastUsed.clear();
	for (auto&& it: blockTypes[0].blocks[block].columns) {

		if (it.command->mdCmdUseLastSet || it.command->mdCmdForceRepeat) lastUsed.push_back(it.columnData[0]);
		else lastUsed.push_back(Datablock_Field(it.command));
	}

	for (unsigned i = 1; i < row; i++) {

		for (unsigned j = 0; j < blockTypes[0].commands.size(); j++) {

//			cout << "checking " <<  blockTypes[0].commands[j]->mdCmdName << " at " << block << ":" << row << endl;
			if (blockTypes[0].commands[j]->mdCmdUseLastSet && blockTypes[0].blocks[block].columns[j].columnData[i].dataString != "")
				lastUsed[j] = blockTypes[0].blocks[block].columns[j].columnData[i];
//				  cout << "updating " << blockTypes[0].commands[j]->mdCmdName << "to " << lastUsed[j].dataString << endl; }
		}
	}
}


void Work_Tune::generate_empty_module(const unsigned &defaultBlockLength) {

	moduleLines.clear();
	moduleLines.push_back("CONFIG=" + configName);
	moduleLines.push_back("");
	moduleLines.push_back(":SEQUENCE");
	moduleLines.push_back("pattern00");
	moduleLines.push_back("");
	moduleLines.push_back(":pattern00");
	for (unsigned i = 0; i < defaultBlockLength; i++) moduleLines.push_back(".");
	moduleLines.push_back("");

	set<string> defaultBlocks;
	for (int i = 0; i < config.mdCmdCount; i++) {

		if (config.mdCmdList[i].isBlkReference) defaultBlocks.insert(config.mdCmdList[i].mdCmdDefaultValString);
	}

	for (auto&& it: defaultBlocks) {

		moduleLines.push_back(":" + it);
		//TODO temp work-around for PhaseSqueek needing STOP command set in fx0 default table
		if (configName == "PhaseSqueek" && it == "fx0") moduleLines.push_back("STOP");
		else moduleLines.push_back(".");
		moduleLines.push_back("");
	}
}


void Work_Tune::generate_module_lines() {

	moduleLines.clear();
	moduleLines.push_back("CONFIG=" + configName);
	for (auto&& it: globalConstants) {
		if (it.command->mdCmdForceString) moduleLines.push_back(it.command->mdCmdName + "=\"" + it.dataString + "\"");
		else moduleLines.push_back(it.command->mdCmdName + "=" + it.dataString);
	}
//	moduleLines.push_back("");
	moduleLines.push_back(":SEQUENCE");
// 	moduleLines.reserve(moduleLines.size() + sequence.size());
// 	moduleLines.insert(moduleLines.end(), sequence.begin(), sequence.end());
	for (size_t i = 0; i < sequence.size(); i++) {

		if (i == sequenceLoopPoint) moduleLines.push_back("\t[LOOP]");
		moduleLines.push_back("\t" + sequence[i]);
	}


	for (auto&& bt: blockTypes) {

		for (auto&& blk: bt.blocks) {

//			moduleLines.push_back("");
			moduleLines.push_back(":" + blk.name);

			for (size_t row = 0; row < blk.columns[0].columnData.size(); row++) {

				string dataString = "";
				bool begin = true;

				for (size_t col = 0; col < blk.columns.size(); col++) {

					if (blk.columns[col].columnData[row].dataString != "") {

						if (begin) dataString += "\t";
						else dataString += ", ";
						if (bt.commands[col]->mdCmdAuto) dataString += bt.commands[col]->mdCmdName;
						else dataString += (bt.commands[col]->mdCmdName + "=" + blk.columns[col].columnData[row].dataString);
						begin = false;
					}
				}

				if (dataString == "") dataString = "\t.";
				moduleLines.push_back(dataString);
			}
		}
	}

//	moduleLines.push_back("");
}


void Work_Tune::generate_pattern_lines(unsigned pattern) {

	patternLines.clear();
	patternLines.push_back("CONFIG=" + configName);
	for (auto&& it: globalConstants) {
		if (it.command->mdCmdForceString) patternLines.push_back(it.command->mdCmdName + "=\"" + it.dataString + "\"");
		else patternLines.push_back(it.command->mdCmdName + "=" + it.dataString);
	}
//	patternLines.push_back("");
	patternLines.push_back(":SEQUENCE");
	patternLines.push_back("\t[LOOP]");
	patternLines.push_back("\ttest_pattern0000");
//	patternLines.push_back("");
	patternLines.push_back(":test_pattern0000");


	for (size_t row = 0; row < blockTypes[0].blocks[pattern].columns[0].columnData.size(); row++) {

		string dataString = "";
		bool begin = true;

		for (size_t col = 0; col < blockTypes[0].blocks[pattern].columns.size(); col++) {

			if (blockTypes[0].blocks[pattern].columns[col].columnData[row].dataString != "") {

				if (begin) dataString += "\t";
				else dataString += ", ";

				if (blockTypes[0].commands[col]->mdCmdAuto) dataString += blockTypes[0].commands[col]->mdCmdName;
				else dataString += (blockTypes[0].commands[col]->mdCmdName + "="
					+ blockTypes[0].blocks[pattern].columns[col].columnData[row].dataString);
				begin = false;
			}
		}

		if (dataString == "") dataString = "\t.";
		patternLines.push_back(dataString);
	}


	for (size_t bt = 1; bt < blockTypes.size(); bt++) {

		for (auto&& blk: blockTypes[bt].blocks) {

//			patternLines.push_back("");
			patternLines.push_back(":" + blk.name);

			for (size_t row = 0; row < blk.columns[0].columnData.size(); row++) {

				string dataString = "";
				bool begin = true;

				for (size_t col = 0; col < blk.columns.size(); col++) {

					if (blk.columns[col].columnData[row].dataString != "") {

						if (begin) dataString += "\t";
						else dataString += ", ";
						if (blockTypes[bt].commands[col]->mdCmdAuto) dataString += blockTypes[bt].commands[col]->mdCmdName;
						else dataString += (blockTypes[bt].commands[col]->mdCmdName + "="
							+ blk.columns[col].columnData[row].dataString);
						begin = false;
					}
				}

				if (dataString == "") dataString = "\t.";
				patternLines.push_back(dataString);
			}
		}
	}

//    for (auto&& it: patternLines) cout << it << endl;
//	patternLines.push_back("");
}


vector<string> Work_Tune::generate_lines_from_pos(const unsigned &block, const unsigned &row_, const unsigned &sequencePos) {

    vector<string> mdLines;

    mdLines.push_back("CONFIG=" + configName);
    for (auto&& it: globalConstants) {
		if (it.command->mdCmdForceString) mdLines.push_back(it.command->mdCmdName + "=\"" + it.dataString + "\"");
		else mdLines.push_back(it.command->mdCmdName + "=" + it.dataString);
	}

    mdLines.push_back(":SEQUENCE");
    mdLines.push_back("\t[LOOP]");
    mdLines.push_back("\ttest_pattern0000");
// 	moduleLines.reserve(moduleLines.size() + sequence.size());
// 	moduleLines.insert(moduleLines.end(), sequence.begin(), sequence.end());
	for (size_t i = sequencePos + 1; i < sequence.size(); i++) mdLines.push_back("\t" + sequence[i]);

	mdLines.push_back(":test_pattern0000");

    string dataString = "";
	bool begin = true;

	for (size_t col = 0; col < blockTypes[0].commands.size(); col++) {

		if (blockTypes[0].blocks[block].columns[col].columnData[row_].dataString != "") {

			if (begin) dataString += "\t";
			else dataString += ", ";
			if (blockTypes[0].commands[col]->mdCmdAuto) dataString += blockTypes[0].commands[col]->mdCmdName;
			else dataString += (blockTypes[0].commands[col]->mdCmdName + "="
				+ blockTypes[0].blocks[block].columns[col].columnData[row_].dataString);
			begin = false;
		}

		else if (lastUsed[col].dataString != "") {
//			cout << "trig on cmd " << blockTypes[0].commands[col]->mdCmdName << endl;
			if (begin) dataString += "\t";
			else dataString += ", ";
			if (blockTypes[0].commands[col]->mdCmdAuto) dataString += blockTypes[0].commands[col]->mdCmdName;
			else dataString += (blockTypes[0].commands[col]->mdCmdName + "=" + lastUsed[col].dataString);
			begin = false;
		}
	}

	if (dataString == "") dataString = "\t.";
    mdLines.push_back(dataString);

	for (size_t row = row_ + 1; row < blockTypes[0].blocks[block].columns[0].columnData.size(); row++) {

        dataString = "";
        begin = true;

        for (size_t col = 0; col < blockTypes[0].blocks[block].columns.size(); col++) {

            if (blockTypes[0].blocks[block].columns[col].columnData[row].dataString != "") {

                if (begin) dataString += "\t";
                else dataString += ", ";
                if (blockTypes[0].commands[col]->mdCmdAuto) dataString += blockTypes[0].commands[col]->mdCmdName;
                else dataString += (blockTypes[0].commands[col]->mdCmdName + "="
                    + blockTypes[0].blocks[block].columns[col].columnData[row].dataString);
                begin = false;
            }
        }

        if (dataString == "") dataString = "\t.";
        mdLines.push_back(dataString);
	}


	for (auto&& bt: blockTypes) {

		for (auto&& blk: bt.blocks) {

			mdLines.push_back(":" + blk.name);

			for (size_t row = 0; row < blk.columns[0].columnData.size(); row++) {

				dataString = "";
				begin = true;

				for (size_t col = 0; col < blk.columns.size(); col++) {

					if (blk.columns[col].columnData[row].dataString != "") {

						if (begin) dataString += "\t";
						else dataString += ", ";
						if (bt.commands[col]->mdCmdAuto) dataString += bt.commands[col]->mdCmdName;
						else dataString += (bt.commands[col]->mdCmdName + "=" + blk.columns[col].columnData[row].dataString);
						begin = false;
					}
				}

				if (dataString == "") dataString = "\t.";
				mdLines.push_back(dataString);
			}
		}
	}

    return mdLines;
}


void Work_Tune::generate_single_line(const unsigned &block, const unsigned &row) {

	singleLine.clear();
	singleLine.push_back("CONFIG=" + configName);
	for (auto&& it: globalConstants) {
		if (it.command->mdCmdForceString) singleLine.push_back(it.command->mdCmdName + "=\"" + it.dataString + "\"");
		else singleLine.push_back(it.command->mdCmdName + "=" + it.dataString);
	}

	singleLine.push_back(":SEQUENCE");
	singleLine.push_back("\t[LOOP]");
	singleLine.push_back("\ttest_pattern0000");
	singleLine.push_back(":test_pattern0000");


	string dataString = "";
	bool begin = true;

	for (size_t col = 0; col < blockTypes[0].commands.size(); col++) {

		if (blockTypes[0].blocks[block].columns[col].columnData[row].dataString != "") {

			if (begin) dataString += "\t";
			else dataString += ", ";
			if (blockTypes[0].commands[col]->mdCmdAuto) dataString += blockTypes[0].commands[col]->mdCmdName;
			else dataString += (blockTypes[0].commands[col]->mdCmdName + "="
				+ blockTypes[0].blocks[block].columns[col].columnData[row].dataString);
			begin = false;
		}

		else if (lastUsed[col].dataString != "") {
//			cout << "trig on cmd " << blockTypes[0].commands[col]->mdCmdName << endl;
			if (begin) dataString += "\t";
			else dataString += ", ";
			if (blockTypes[0].commands[col]->mdCmdAuto) dataString += blockTypes[0].commands[col]->mdCmdName;
			else dataString += (blockTypes[0].commands[col]->mdCmdName + "=" + lastUsed[col].dataString);
			begin = false;
		}
	}

	if (dataString == "") dataString = "\t.";
//	cout << dataString << endl;
	singleLine.push_back(dataString);

	//TODO: add non-pattern blocks

	for (unsigned bt = 1; bt < blockTypes.size(); bt++) {

		for (auto&& blk: blockTypes[bt].blocks) {

			singleLine.push_back(":" + blk.name);

			for (size_t bRow = 0; bRow < blk.columns[0].columnData.size(); bRow++) {

				dataString = "";
				begin = true;

				for (size_t col = 0; col < blk.columns.size(); col++) {

					if (blk.columns[col].columnData[bRow].dataString != "") {

						if (begin) dataString += "\t";
						else dataString += ", ";
						if (blockTypes[bt].commands[col]->mdCmdAuto) dataString += blockTypes[bt].commands[col]->mdCmdName;
						else dataString += (blockTypes[bt].commands[col]->mdCmdName + "="
							+ blk.columns[col].columnData[bRow].dataString);
						begin = false;
					}
				}

				if (dataString == "") dataString = "\t.";
				singleLine.push_back(dataString);
			}
		}
	}

//	for (auto&& it: singleLine) cout << it << endl;		//debug
}

string Work_Tune::generate_unique_block_name() {

	string unique = "blk";
	stringstream seed;
	seed << uniqueSeed;

	while (!is_block_name_unique(string(unique + seed.str()))) {

		uniqueSeed++;
		if (uniqueSeed > 0xffff) throw (string("Could not generate unique block name."));
		seed.str(string());
		seed << uniqueSeed;
	}

	return (unique + seed.str());
}

bool Work_Tune::is_block_name_unique(const string &blockName) {

	for (auto&& bt: blockTypes) {

		for (auto&& blk: bt.blocks) {

			if (blk.name == blockName) return false;
		}
	}

	return true;
}

unsigned Work_Tune::get_frequency_divider(const string &noteName) {

    if (musicdataBinary.equates.count(noteName)) return musicdataBinary.equates[noteName];
    else return 0;
}

unsigned Work_Tune::get_note_index(const string &noteName) {

    string sOct = "";
    sOct.push_back(noteName.back());
    if (!isNumber(sOct)) throw (string("Invalid note name"));
    unsigned nOct = strToNum(sOct);
    string baseName = noteName.substr(0, noteName.size() - 1);

    unsigned index;
    for (index = 0; index < 12 && baseName != noteNames[index]; index++) {}
    if (index == 12) throw (string("Invalid note name"));

    return ((nOct * 12) + index);
}


string Work_Tune::get_note_data_string(const unsigned &freqDivider) {

    string dataString = "";

    unsigned index;
    for (index = 0; index < freqDividers.size() && freqDivider >= freqDividers[index]; index++) {};

    int offset = 0;
    if (index == 0) dataString = "0+" + numToStr(offset, 0, false);
    else {
        index--;
        offset = freqDivider - freqDividers[index];
        string nname = noteNames[index % 12];
        unsigned nOct = index / 12;

        dataString = nname + numToStr(nOct, 0, false);
        if (offset != 0) dataString += "+" + numToStr(offset, 0, false);
    }

    return dataString;
}


vector<char> Work_Tune::reload_data(const int &playMode) {

//    cout << "called\n";
    vector<char> mdata;

    if (playMode == PM_PATTERN) {

        //TODO: this will break once more than 1 pattern blocktype is allowed
        generate_pattern_lines(status->currentBlocks[0]);
        bool verbose = false;
        mdModule mdf(patternLines, config, verbose);

        //TODO: TEMP convert sstream to vector
        vector<string> asmLines;
        istringstream is(mdf.MUSICASM.str());
        string tempstr;
        while (getline(is, tempstr)) asmLines.push_back(tempstr);

        //TODO: looping/no looping
        musicdataBinary.assemble(asmLines, orgAddress + engineSize, true);

        if (orgAddress + engineSize + musicdataBinary.binLength > 0xfff0)
            throw (string("Error: Music Data too large."));

        mdata.reserve(musicdataBinary.binLength);
        for (long i = 0; i < musicdataBinary.binLength; i++)
            mdata.push_back(musicdataBinary.binData[static_cast<unsigned>(i)]);
    }

    else if (playMode == PM_SONG) {

        generate_module_lines();
        bool verbose = false;
        mdModule mdf(moduleLines, config, verbose);

        //TODO: TEMP convert sstream to vector
        vector<string> asmLines;
        istringstream is(mdf.MUSICASM.str());
        string tempstr;
        while (getline(is, tempstr)) asmLines.push_back(tempstr);

        //TODO: looping/no looping
        musicdataBinary.assemble(asmLines, orgAddress + engineSize, true);

//        //if a valid loop patch address was registered, patch loop point
//        //TODO this has currently no effect
//        if (loopPatchAddr != -1) {
//            long lp = musicdataBinary.symbols.at(config.seqLoopLabel);
//            engineCode[loopPatchAddr] = static_cast<char>(lp);
//            engineCode[loopPatchAddr + 1] = static_cast<char>(lp >> 8);
//        }

        mdata.reserve(musicdataBinary.binLength);
        for (long i = 0; i < musicdataBinary.binLength; i++)
            mdata.push_back(musicdataBinary.binData[static_cast<unsigned>(i)]);

//        soundEmul.vm->load_raw_data(mdata, currentTune.orgAddress + currentTune.engineSize);

    }

    return mdata;
}


