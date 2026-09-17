#include "SA3Tokenizer.h"
#include <fstream>
#include <iostream>

SA3Tokenizer::SA3Tokenizer(const std::filesystem::path &modelsDir)
{
	tokenizerPath = modelsDir / Obsidian::TOKENIZER();
	auto blob = LoadBytesFromFile(tokenizerPath);
	tokenizer = tokenizers::Tokenizer::FromBlobJSON(blob);
}

SA3Tokenizer::~SA3Tokenizer()
{
}

std::string SA3Tokenizer::LoadBytesFromFile(std::filesystem::path &path)
{
	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	return content;
}

SA3Tokenizer::TokenizerOutput SA3Tokenizer::encode(std::string &prompt)
{
	if (tokenizer)
	{
		std::vector<int> ids = tokenizer->Encode(prompt);
		std::vector<int> mask(ids.size(), 1);
		int maxLength = 256;
		int padTokenId = 0;
		mask.resize(maxLength, 0);
		ids.resize(maxLength, padTokenId);

		return {ids, mask};
	}
	return {};
}