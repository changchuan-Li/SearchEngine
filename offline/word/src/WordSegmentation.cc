#include "../include/WordSegmentation.h"

vector<string> WordSegmentation::operator()(const string str)
{
    vector<string> words;
    _jieba.CutAll(str, words); //FullSegment
    return words;
}

void WordSegmentation::CutSmall(const string & str, vector<string> & vec, size_t sz)
{
    _jieba.CutSmall(str, vec, sz);
}
