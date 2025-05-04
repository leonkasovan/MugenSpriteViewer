/*
Parse AIR file format for Mugen
This program parses the AIR file format used in Mugen, a 2D fighting game engine.
It extracts animation frames and collision boxes from the file and prints them to the console.

Compile:
g++ -std=c++17 -o parse_air src/parse_air.cpp
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <map>
#include <cstdint> // For SIZE_MAX

struct CollisionBox {
	int x1, y1, x2, y2;
};

struct AnimFrame {
	int group = -1, number = -1;
	int xoffset = 0, yoffset = 0;
	int time = -1;
	int hscale = 1, vscale = 1;
	float xscale = 1.0f, yscale = 1.0f;
	int srcAlpha = 255, dstAlpha = 0;
	float angle = 0.0f;
	std::vector<std::vector<CollisionBox>> clsn;
};

struct Action {
	int number;
	std::vector<AnimFrame> frames;
	int loopstart = -1;
};

bool isNumeric(const std::string& s) {
	if (s.empty()) return false;
	char* end;
	strtof(s.c_str(), &end);
	return end != s.c_str();
}

void trim(std::string& s) {
	auto it = std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); });
	s.erase(s.begin(), it);
	auto rit = std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); });
	s.erase(rit.base(), s.end());
}

std::vector<std::string> split(const std::string& s, char delim, size_t maxParts = SIZE_MAX) {
	std::vector<std::string> parts;
	std::stringstream ss(s);
	std::string item;
	size_t count = 0;
	while (std::getline(ss, item, delim) && count++ < maxParts) {
		trim(item);
		parts.push_back(item);
	}
	return parts;
}

AnimFrame parseAnimFrame(const std::string& line) {
	AnimFrame af;
	auto parts = split(line, ',', 10);
	if (parts.size() < 5) return af;
	af.group = std::stoi(parts[0]);
	af.number = std::stoi(parts[1]);
	af.xoffset = std::stoi(parts[2]);
	af.yoffset = std::stoi(parts[3]);
	af.time = std::stoi(parts[4]);

	if (parts.size() >= 6) {
		std::string flips = parts[5];
		std::transform(flips.begin(), flips.end(), flips.begin(), ::tolower);
		if (flips.find('h') != std::string::npos) {
			af.hscale = -1;
			af.xoffset *= -1;
		}
		if (flips.find('v') != std::string::npos) {
			af.vscale = -1;
			af.yoffset *= -1;
		}
	}

	if (parts.size() >= 7) {
		std::string alpha = parts[6];
		std::transform(alpha.begin(), alpha.end(), alpha.begin(), ::tolower);
		if (alpha == "a1") {
			af.srcAlpha = 255;
			af.dstAlpha = 128;
		} else if (alpha == "a") {
			af.srcAlpha = 255;
			af.dstAlpha = 255;
		} else if (alpha.rfind("as", 0) == 0) {
			size_t dpos = alpha.find('d');
			if (dpos != std::string::npos) {
				int src = std::stoi(alpha.substr(2, dpos - 2));
				int dst = std::stoi(alpha.substr(dpos + 1));
				af.srcAlpha = std::clamp(src, 0, 255);
				af.dstAlpha = std::clamp(dst, 0, 255);
			}
		}
	}

	if (parts.size() >= 8 && isNumeric(parts[7])) af.xscale = std::stof(parts[7]);
	if (parts.size() >= 9 && isNumeric(parts[8])) af.yscale = std::stof(parts[8]);
	if (parts.size() >= 10 && isNumeric(parts[9])) af.angle = std::stof(parts[9]);

	return af;
}

std::map<int, Action> parseAirFile(const std::string& filename) {
	std::ifstream file(filename);
	std::string line;
	std::map<int, Action> actions;
	Action* current = nullptr;
	int clsn2default = 0;
	std::vector<CollisionBox> clsn2Defaults;

	for (int lineno = 1; std::getline(file, line); ++lineno) {
		auto semicolon = line.find(';');
		if (semicolon != std::string::npos) line = line.substr(0, semicolon);
		trim(line);
		if (line.empty()) continue;

		std::string lower = line;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

		if (lower.find("[begin action") != std::string::npos) {
			int actNum = std::stoi(line.substr(line.find_last_of(' ') + 1));
			current = &actions[actNum];
			current->number = actNum;
			clsn2default = 0;
			clsn2Defaults.clear();
		} else if (!current) {
			continue;
		} else if (lower.find("clsn2default") != std::string::npos) {
			clsn2default = std::stoi(line.substr(line.find(':') + 1));
			clsn2Defaults.clear();
		} else if (lower.find("clsn2[") != std::string::npos) {
			auto eq = line.find('=');
			auto coords = split(line.substr(eq + 1), ',');
			if (coords.size() == 4) {
				CollisionBox box;
				box.x1 = std::stoi(coords[0]);
				box.y1 = std::stoi(coords[1]);
				box.x2 = std::stoi(coords[2]);
				box.y2 = std::stoi(coords[3]);
				clsn2Defaults.push_back(box);
			}
		} else if (lower == "loopstart") {
			if (current) current->loopstart = current->frames.size();
		} else if (std::isdigit(line[0]) || line[0] == '-') {
			auto frame = parseAnimFrame(line);
			if (clsn2default > 0 && clsn2Defaults.size() == clsn2default) {
				frame.clsn.resize(2);
				frame.clsn[1] = clsn2Defaults;
			}
			current->frames.push_back(frame);
		}
	}

	return actions;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: airparser <file.air>\n";
		return 1;
	}

	auto actions = parseAirFile(argv[1]);
	for (const auto& [num, action] : actions) {
		std::cout << "Action " << num << " (frames: " << action.frames.size() << ")\n";
		if (action.loopstart >= 0)
			std::cout << "  Loop starts at frame index: " << action.loopstart << "\n";
		for (const auto& frame : action.frames) {
			std::cout << "  Sprite: " << frame.group << "," << frame.number
				<< " Offset(" << frame.xoffset << "," << frame.yoffset << ")"
				<< " Time: " << frame.time
				<< " Flip: " << (frame.hscale == -1 ? "H" : "") << (frame.vscale == -1 ? "V" : "")
				<< " Alpha: " << frame.srcAlpha << "/" << frame.dstAlpha
				<< " Scale: " << frame.xscale << "," << frame.yscale
				<< " Angle: " << frame.angle << "\n";
		}
	}

	return 0;
}
