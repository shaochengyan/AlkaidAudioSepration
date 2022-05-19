#include "AlkaidConvTasNet.h"
#include <time.h>

// constructor
AlkaidConvTasNet::AlkaidConvTasNet()
    : session_options(_GetSessionOptions()),
      sess1(env, L"./model/Alkaid_M1.onnx", session_options),
      sess2(env, L"./model/Alkaid_M2.onnx", session_options),
      mem_info(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator,
                                          OrtMemType::OrtMemTypeDefault)),
      input_tensor_names{"input"},
      output_tensor_names{"output"} {

          // �����Ż�
          // 1 ���д洢
      }


AlkaidConvTasNet::AlkaidConvTasNet(wchar_t* model_1, wchar_t* model_2)
    : session_options(_GetSessionOptions()),
    sess1(env, model_1, session_options),
    sess2(env, model_2, session_options),
    mem_info(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator,
        OrtMemType::OrtMemTypeDefault)),
    input_tensor_names{ "input" },
    output_tensor_names{ "output" } {
}

// Run1
// ��model1 input:audio_mix_both output: (audio_mix_clean, audio_noise)
// ����ֻ�����ݽ��в��������������ͷ����µ�����
// Ĭ�ϣ�
long long AlkaidConvTasNet::RunModel(Ort::Session& sess,
                                std::vector<float>& input_vec,
                                std::vector<float>& output_vec,
                                std::vector<int64_t>& input_tensor_shape,
                                std::vector<int64_t>& output_tensor_shape) {
    // input tensor
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        mem_info, input_vec.data(), input_vec.size(), input_tensor_shape.data(),
        input_tensor_shape.size());
    // output tensor
    output_vec.resize(VecProduct(output_tensor_shape));
    Ort::Value output_tensor = Ort::Value::CreateTensor<float>(
        mem_info, output_vec.data(), output_vec.size(),
        output_tensor_shape.data(), output_tensor_shape.size());
    // run model 1
    COLA_RECORD_TIME(
        sess.Run(Ort::RunOptions(nullptr), input_tensor_names.data(), &input_tensor,
            1, output_tensor_names.data(), &output_tensor, 1);, time_latency);
    return time_latency;  // ʱ�� long long ����
}

void AlkaidConvTasNet::TestLatencyWithTimelong(int timelong) {
    // ����ģ����Ƶ
    int audio_len = timelong * 16000;
    std::vector<float> input_vec(audio_len);
    for (uint32_t idx = 0; idx < audio_len; ++idx) {
        input_vec[idx] = (rand() % 100) / 50 - 1;
    }
    std::vector<float> output_vec(audio_len);

    // ��������
    std::vector<int64_t> input_tensor_shape = { audio_len };
    std::vector<int64_t> output_tensor_shape = { 2, audio_len };

    // ����ģ��1
    COLA_MEASER_TIME(
        RunModel(sess1, input_vec, output_vec, input_tensor_shape,
            output_tensor_shape); , "M1 Latency of " << timelong << " sec = ");

    // ����ģ��2
    COLA_MEASER_TIME(
        RunModel(sess2, input_vec, output_vec, input_tensor_shape,
            output_tensor_shape);, "M2 Latency of " << timelong << " sec = ")

}


Ort::SessionOptions AlkaidConvTasNet::_GetSessionOptions() {
    Ort::SessionOptions session_options;
    // ��������
    session_options.SetLogSeverityLevel(3);

    // �Ż�1
    session_options.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_ALL);

    // �Ż�2������ִ��
    // session_options.SetExecutionMode(ExecutionMode::ORT_PARALLEL);
    return session_options;
}

// Run
// ���ݲ�ͬ��type�������в�ͬ�ķ�֧run
/*
    shape�ڴ�����Ƶ֮��Ϳ���ֱ�Ӵ���

    �㷨ǰ��
    sess1��sess2��ģ����������ӿ���ȫһ�£�
*/
void AlkaidConvTasNet::Run(const char* filename, int32_t n_src, bool is_noisy) {
    // debug
    printf("=====Processing:%s=====\n", filename);
    // ·������
    cpu.Update(filename, ".wav");
    // �洢��Ƶ������
    std::vector<float> input_vec;
    std::vector<float> output_vec;
    // ������Ƶ������shape -> todo: ����if����
    AudioUtils au;
    au.LoadAudio(filename, input_vec);
    int64_t audio_len = input_vec.size();
    // tensor shape
    std::vector<int64_t> input_tensor_shape = {audio_len};
    std::vector<int64_t> output_tensor_shape = {2, audio_len};
    // �洢��Ƶ���ļ���
    int ret = _access(cpu.Postfix("/"), 0);
    _mkdir(cpu.Postfix("/"));
    // ���������
    long long latency = 0;
    if (n_src == 1 && is_noisy) {  // (audio_mix_both) -model1->
                                   // (audio_mix_clean, audio_noise)
        latency = RunModel(sess1, input_vec, output_vec, input_tensor_shape,
                 output_tensor_shape);
        printf("M1 Latency = %d (ms)\n", latency);
        au.DumpAudio(cpu.Postfix("/clean.wav"), output_vec.data(), audio_len,
                     -1024);
        au.DumpAudio(cpu.Postfix("/noise.wav"), output_vec.data() + audio_len,
                     audio_len, -4096);
    } else if (n_src == 2 && !is_noisy) {  // (audio_mix_clean/audio_mix_both)
                                           // -model2-> (audio_src1, audio_src2)
        latency = RunModel(sess2, input_vec, output_vec, input_tensor_shape,
                 output_tensor_shape);
        printf("M2 Latency = %d (ms)\n", latency);
        au.DumpAudio(cpu.Postfix("/src1.wav"), output_vec.data(), audio_len,
                     -64);
        au.DumpAudio(cpu.Postfix("/src2.wav"), output_vec.data() + audio_len,
                     audio_len, -64);
    }
#define test_parall
#ifndef test_parall
    else if (n_src == 2 && is_noisy) {  // �������������������
        // model1����
        std::thread thread_sep_noise([&]() {
            RunModel(sess1, input_vec, output_vec, input_tensor_shape,
                     output_tensor_shape);
            // au.DumpAudio(cpu.Postfix("/clean.wav"), output_vec.data(),
            //              audio_len, -512);
            au.DumpAudio(cpu.Postfix("/noise.wav"),
                         output_vec.data() + audio_len, audio_len, -4096);
        });
        // model2 ����
        std::vector<float> output_vec2;
        std::thread thread_sep_clean([&]() {
            RunModel(sess2, input_vec, output_vec2, input_tensor_shape,
                     output_tensor_shape);
            au.DumpAudio(cpu.Postfix("/src1.wav"), output_vec2.data(),
                         audio_len, -128);
            au.DumpAudio(cpu.Postfix("/src2.wav"),
                         output_vec2.data() + audio_len, audio_len, -128);
        });

        // join
        thread_sep_noise.join();
        thread_sep_clean.join();
        return;
    }
#else
    else if (n_src == 2 && is_noisy) {  // �������������������
        // model1����
        latency = RunModel(sess1, input_vec, output_vec, input_tensor_shape,
                 output_tensor_shape);
        printf("M1 Latency = %d (ms)\n", latency);
        au.DumpAudio(cpu.Postfix("/noise.wav"), output_vec.data() + audio_len,
                     audio_len, -4096);  // �洢����
        output_vec.resize(audio_len);    // ������mix_clean
        for(auto it = output_vec.begin(); it != output_vec.end(); ++it) {
            *it /= 128;
        }
        // model2 ����
        std::vector<float>
            output_vec2;  // cola_todo: ����ֱ��ʹ��input_vec�����
        latency = RunModel(sess2, output_vec, output_vec2, input_tensor_shape,
                 output_tensor_shape);
        printf("M2 Latency = %d (ms)\n", latency);
        au.DumpAudio(cpu.Postfix("/src1.wav"), output_vec2.data(), audio_len,
                     -64);
        au.DumpAudio(cpu.Postfix("/src2.wav"), output_vec2.data() + audio_len,
                     audio_len, -64);
    }
#endif  // !test_parall

    printf("=====End=====\n");
}