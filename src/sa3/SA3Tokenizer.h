#pragma once
#include "DataConst.h"
#include <filesystem>
#include <tokenizers_cpp.h>
#include <vector>

class SA3Tokenizer
{
  public:
	SA3Tokenizer(const std::filesystem::path &modelsDir);
	~SA3Tokenizer();

	struct TokenizerOutput
	{
		std::vector<int> inputIds;
		std::vector<int> attentionMask;
	};

	TokenizerOutput encode(std::string &prompt);

  private:
	std::unique_ptr<tokenizers::Tokenizer> tokenizer;

	std::filesystem::path tokenizerPath;

	std::string LoadBytesFromFile(std::filesystem::path &path);
};