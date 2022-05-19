#include "AudioUtils.h"

bool AudioUtils::LoadAudio(const char* filename, std::vector<float>& audioVec) {
    // ���ļ�
    FILE* fin;
    errno_t is_OpenError = fopen_s(&fin, filename, "rb");
    if (is_OpenError != 0) {
        throw "error: Wav file open error.";
    }

    // ��ȡheader
    fread(&header, sizeof(wav_header_t), 1, fin);

    // ���ļ�ʱwav��ʽ��Ƶ��Ϊ16kHz��Ҫ��
    if (!strcmp(header.format, "WAVE") || header.sampleRate != 16e3) {
        throw "Audio file must be .wav format and sample rate equal to 16kHz";
    }


    // ��ȡ chunk ��ȷ�����ݿ�Ŀ�ʼ����
    while (true) {
        fread(&chunk, sizeof(chunk_t), 1, fin);  // �ȶ�ȡ���ݿ�
        // "data"�ַ��� �Լ��������ݿ�Ĵ�С
        // printf("%c%c%c%c\t" "%li\n", chunk.ID[0], chunk.ID[1], chunk.ID[2],
        // chunk.ID[3], chunk.size);
        if (*(unsigned int*)&chunk.ID == 0x61746164)
            break;
        // �����ȡ����data.ID != 'data' ��������Զ�
        fseek(fin, chunk.size, SEEK_CUR);
    }

    int byte_per_sample = 2;  // Ĭ����16λ���� -> todo���԰��ղ�ͬ������޸�
    int sample_cnt = chunk.size / byte_per_sample;  // �ܹ���������
    audioVec.resize(sample_cnt);

    std::vector<float>::iterator it = audioVec.begin();
    short int tmp_16bits = 0;  // �洢һ�����ݵ�
    for (unsigned int i = 0; i < sample_cnt; ++i, ++it) {
        fread(&tmp_16bits, sizeof(tmp_16bits), 1, fin);
        *it = (tmp_16bits) * -1.0f / POW2_15;
        //printf("%f \t %d\n", *it, tmp_16bits);
    }

    return true;
}

bool AudioUtils::DumpAudio(const char* filename,
                           const std::vector<float>& audioVec, 
                           const short int scale) {
    return this->DumpAudio(filename, audioVec.data(), audioVec.size(), scale);
}

bool AudioUtils::DumpAudio(const char* filename,
                           const float* ptr,
                           uint32_t len, 
                           const short int scale) {
    FILE* fout;
    errno_t isOpenError = fopen_s(&fout, filename, "wb");  // 0 ��û�д���

    if (isOpenError) {
        throw "error: Audio file open error when dummping!";
    }
    COLA_SHOW_INFO("Output filename=", filename);

    fwrite(&header, sizeof(header), 1, fout);
    fwrite(&chunk, sizeof(chunk), 1, fout);

    // д������
    short int tmp;
    for (uint32_t idx = 0; idx < len; ++idx) {
        //tmp = ptr[idx] / 100 * -POW2_15;
        tmp = ptr[idx] * scale;  // ���ڴ��ڳ���1�����ݣ������������1 << 15��ᵼ�����
        //printf("%f \t %d\n", ptr[idx], tmp);
        fwrite(&tmp, sizeof(tmp), 1, fout);
    }

    fclose(fout);
    return true;
}
