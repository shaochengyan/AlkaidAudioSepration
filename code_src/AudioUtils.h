#pragma once
#include <assert.h>
#include <string>
#include <vector>
#include "ColaUtils.h"

// wav�ļ��ֶ�˵��
struct wav_header_t {
    char chunkID[4];  // 4 byte: ASCII���ʾ��ʽ(RIFF)
    unsigned long chunkSize;  // 4 byte �����ļ���С �����������ID��SIZE����
    char format[4];               // 4 byte һ����WAVE
    char subchunk1ID[4];          // 4 ��ʽ˵���� һ���� "fmt "
    unsigned long subchunk1Size;  // �����ݿ��С
    unsigned short audioFormat;   // ��Ƶ��ʽ˵��
    unsigned short numChannels;   // ������
    unsigned long sampleRate;     // ������ ..HZ
    unsigned long byteRate;       // ������ f * n = f * 8|16
    unsigned short blockAlign;  // ���ݶ��뵥Ԫ -> ˫ͨ�� �� ÿ��������ռ2��byte
                                // ����䳤��Ϊ4 byte
    unsigned short
        bitsPerSample;  // �������������� 8 | 16 bits -> ��ӳ�ֱ��ʴ�С
};

// ���� ��
struct chunk_t {
    char ID[4];  // "data" ��ASCII��ֵ = 0x61746164  <- d(64) a(61) t(74) a(61)
                 // d�����λ
    unsigned long size;  // ��Chunk���ܴ�С
};

const short int POW2_15 = -((short int)(1 << 15));
const short int POW2_8 = -((short int)(1 << 8));

class AudioUtils {
   private:
    // ��Ƶ���ݣ��ڴ洢��ʱ����Ҫʹ��
    unsigned long samples_count;
    wav_header_t header;
    chunk_t chunk;

   public:
    bool LoadAudio(const char* filename, std::vector<float>& audioVec);
    bool DumpAudio(const char* filename, const std::vector<float>& audioVec, const short int scale=POW2_8);
    bool DumpAudio(const char* filename, const float* ptr, uint32_t len, const short int scale=POW2_8);
    
};
