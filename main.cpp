
#include "config.h"

#include <iostream>
#include <fstream>
#include <unordered_set>
#include <vector>
#include <string>
#include <array>
#include <thread>
#include <chrono>
#include <algorithm>
#include <string_view>
#include <stack>
#include <iomanip>

#define BIT(index) (1 << index)

using Clock = std::chrono::high_resolution_clock;
Clock::time_point globalStart;
size_t globalProgress;
size_t globalChecks;

const size_t wordLength = 5;
char* globalWordBlob;
size_t globalTotalWords;
std::vector<int> globalAllWords;
std::vector<std::vector<int>> globalNeighbors;
std::vector<std::vector<int>> globalAllCombinations;

std::chrono::milliseconds TimeElapsed()
{
	Clock::time_point now = Clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now - globalStart);
}

std::string Duration(size_t aMillis)
{
	if (aMillis < 1000)
		return std::to_string(aMillis) + "ms";

	if (aMillis < 1000 * 60)
		return std::to_string(aMillis / 1000) + "s";

	if (aMillis < 1000 * 60 * 60)
		return std::to_string(aMillis / 1000 / 60) + "m";

	return std::to_string(aMillis / 1000 / 60 / 60) + "h";
}

std::string TimeStamp()
{
	return "[" + Duration(static_cast<size_t>(TimeElapsed().count())) + "]";
}

void ReportCombination(std::vector<int> aCombination)
{
	globalAllCombinations.push_back(aCombination);
	std::cout << "\r" << TimeStamp() << " Found combination: ";
	for (int& word : aCombination)
		std::cout << " " << globalWordBlob + word * (wordLength + 1);
	std::cout << "                                                    " << std::endl; // overwrite potential progress bar scraps left in console
}

void FindAll(int aDepth, const std::vector<int>& aAvailableNeighboors, std::vector<int>& aProgress, int searchFrom)
{
	globalChecks++;

	if (aDepth == 5)
	{
		ReportCombination(aProgress);
		return;
	}

	for (const int& wordIndex : aAvailableNeighboors)
	{
		if (searchFrom > wordIndex)
			continue;

		std::vector<int>& possibleNeighboors = globalNeighbors[wordIndex];

		std::vector<int> intersection;
		std::set_intersection(
			std::begin(aAvailableNeighboors), std::end(aAvailableNeighboors),
			std::begin(possibleNeighboors), std::end(possibleNeighboors),
			std::back_inserter(intersection));

		aProgress.push_back(wordIndex);
		FindAll(aDepth + 1, intersection, aProgress, wordIndex + 1);
		aProgress.pop_back();
	}
}

void FindAllWithPrint(int aDepth, const std::vector<int>& aAvailableMatches, std::vector<int>& aProgress, int aCheckFrom)
{
	size_t millisElapsed = TimeElapsed().count();
	float progress = static_cast<float>(globalProgress) / static_cast<float>(globalTotalWords);
	float checksPerSecond = static_cast<float>(globalChecks) / static_cast<float>(millisElapsed) * 1000.f;

	std::cout << "\r" << TimeStamp() << " [";

	for (size_t i = 0; i < 50; i++)
	{
		const char fullnessLookup[5] = { ' ', char(176), char(177), char(178), char(219) };
		int fullness = (progress * 50.f - static_cast<float>(i)) * 4.f;

		std::cout << fullnessLookup[(std::max)(0, (std::min)(fullness, 4))];
	}

	std::cout << "] " << std::fixed << std::setprecision(1) << progress * 100.f << "% " << checksPerSecond / 1000.f << "k checks per second           ";

	FindAll(aDepth, aAvailableMatches, aProgress, aCheckFrom);
}

int main()
{
	globalProgress = 0;
	globalChecks = 0;

	globalStart = Clock::now();

	std::ifstream inFile(configWordlistLocation);
	std::vector<std::string> filteredWords;

	std::cout << TimeStamp() << " Digesting words" << std::endl;
	std::string word;
	while (inFile >> word)
		if (word.size() == wordLength)
			filteredWords.push_back(word);
	

	std::cout << TimeStamp() << " Normalizing characters" << std::endl;
	for (std::string& word : filteredWords)
		for (char& c : word)
			c = std::tolower(c);

	std::cout << TimeStamp() << " Removing words with duplicate characters" << std::endl;

	size_t blobSize = filteredWords.size() * (wordLength + 1);
	globalWordBlob = new char[blobSize];
	memset(globalWordBlob, 0, blobSize);
	size_t wordCount = 0;

	for (std::string& word : filteredWords)
	{
		std::unordered_set<char> letters;
		for (char c : word)
			letters.insert(c);

		if (letters.size() == wordLength)
			memcpy(globalWordBlob + wordCount++ * (wordLength + 1), word.c_str(), wordLength);
	}

	globalTotalWords = wordCount;
	std::vector<std::vector<int>> sets;
	sets.resize(wordCount);

	std::cout << TimeStamp() << " Building meta tables" << std::endl;

	std::vector<size_t> bitMappings;
	bitMappings.resize(wordCount);
	for (size_t wordIndex = 0; wordIndex < wordCount; wordIndex++)
	{
		globalAllWords.push_back(wordIndex);
		size_t bits = 0;
		for (size_t i = 0; i < wordLength; i++)
			bits |= BIT(globalWordBlob[wordIndex * (wordLength + 1) + i] - 'a');

		bitMappings[wordIndex] = bits;
	}

	for (size_t wordAIndex = 0; wordAIndex < wordCount; wordAIndex++)
		for (size_t wordBIndex = wordAIndex; wordBIndex < wordCount; wordBIndex++)
			if ((bitMappings[wordAIndex] & bitMappings[wordBIndex]) == 0)
				sets[wordAIndex].push_back(wordBIndex);

	globalNeighbors = sets;

	Clock::time_point last = globalStart;

	std::cout << TimeStamp() << " Traversing" << std::endl;
	std::vector<int> availableMatches = globalAllWords;

	std::vector<int> wordStack;
	std::stack<std::vector<int>> availableStack;

	std::vector<int> progressWorking;
	progressWorking.reserve(5);

	for (const int& wordIndex : globalAllWords)
	{
		globalProgress++;

		progressWorking.push_back(wordIndex);
		FindAllWithPrint(1, globalNeighbors[wordIndex], progressWorking, wordIndex + 1);
		progressWorking.pop_back();
	}

	std::cout << "\nA Total of " << globalAllCombinations.size() << " combinations found" << std::endl;

	std::ofstream outFile("result.txt");
	for (std::vector<int>& combination : globalAllCombinations)
	{
		for (int word : combination)
		{
			outFile << (globalWordBlob + word * (wordLength + 1)) << " ";
		}
		outFile << "\n";
	}

	delete[] globalWordBlob;
}
