#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "ColaUtils.h"
#include <vector>
#include "AlkaidConvTasNet.h"

int main(int argc, char** argv) {
    // ��������
    InputParser opt(argc, argv);  // .exe 2 1 a.wav b.wav 
    opt.ShowOptions();

    // ģ���������
    wchar_t path1[512];
    wchar_t path2[512]; 
    {
        ColaPathUtils cpu(argv[0], "Alkaid.exe");
        cpu.Postfix("models/Alkaid_M1.onnx");
        cpu.Copy2Wchart(path1);
        cpu.Postfix("models/Alkaid_M2.onnx");
        cpu.Copy2Wchart(path2);
    }    
    AlkaidConvTasNet alkaid(path1, path2);

    // ����ģ�� - ��Ƶtype��Ҫ��wav
    for (uint32_t idx_audio = 0; idx_audio < opt.filenames.size(); ++idx_audio) {
        try {
            COLA_MEASER_PER_TIME(
                alkaid.Run(opt.filenames[idx_audio], opt.n_src, opt.is_noisy); , 1);
        }
        catch (const char* info) {
            printf("%s\n", info);
        }
    }

    return 0;
}
