#include <iostream>
#include <fstream>
#include "AudioFile.h"
#include "json.hpp"
#include <cmath>
#include <vector>
#include <string>
extern "C"
{
#include "kiss_fft.h"
}

using namespace std;
using json = nlohmann::json;


double estimate_frequency(const vector<double>& buffer, int sample_rate) {
    int n = buffer.size();
    if (n == 0) return 0.0;
    
    kiss_fft_cfg cfg = kiss_fft_alloc(n, 0, NULL, NULL);
    vector<kiss_fft_cpx> cx_in(n);
    vector<kiss_fft_cpx> cx_out(n);

    for (int i = 0; i < n; i++) {
        cx_in[i].r = buffer[i];
        cx_in[i].i = 0;
    }

    kiss_fft(cfg, cx_in.data(), cx_out.data());
    free(cfg);

    double max_mag = 0.0;
    int peak_idx = 0;

   
    for (int i = 1; i < n / 2; i++) {
        double mag = sqrt(cx_out[i].r * cx_out[i].r + cx_out[i].i * cx_out[i].i);
        if (mag > max_mag) {
            max_mag = mag;
            peak_idx = i;
        }
    }

    return (double)peak_idx * sample_rate / n;
}

double rms(const vector<double>& buffer) {
    if (buffer.empty()) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < buffer.size(); i++) sum += buffer[i] * buffer[i];
    return sqrt(sum / buffer.size());
}

double CrestFactor(const vector<double>& buffer, double rms_val) {
    if (rms_val == 0) return 0.0;
    double max_val = 0.0;
    for (int i = 0; i < buffer.size(); i++) {
        if (abs(buffer[i]) > max_val) max_val = abs(buffer[i]);
    }
    return max_val / rms_val;
}

double kurtosis(const vector<double>& buffer) {
    if (buffer.empty()) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < buffer.size(); i++) sum += buffer[i];
    double mean = sum / buffer.size();
    double m2 = 0.0, m4 = 0.0;
    for (int i = 0; i < buffer.size(); i++) {
        double dev = buffer[i] - mean;
        m2 += dev * dev;
        m4 += dev * dev * dev * dev;
    }
    m2 /= buffer.size();
    m4 /= buffer.size();
    if (m2 == 0) return 0.0;
    return m4 / (m2 * m2);
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    string wav_path = argv[1];

    AudioFile<double> audio;
    if (!audio.load(wav_path)) return 1;

    vector<double>& data = audio.samples[0];
    int sample_rate = audio.getSampleRate();

    double val_rms = rms(data);
    double val_crest = CrestFactor(data, val_rms);
    double val_kurt = kurtosis(data);
    double rms_db = 20.0 * log10(val_rms + 1e-9);

    
    double peak_freq = estimate_frequency(data, sample_rate);

    double baseline_delta = 0.0;
    bool passed = true;

    if (argc >= 3) {
        string baseline_path = argv[2];
        ifstream f(baseline_path);
        if (f.is_open()) {
            try {
                json baselines = json::parse(f);
                double baseline_rms = 0.0;
                bool found = false;

                if (baselines.contains("quiet.wav") && baselines["quiet.wav"].contains("rms_db")) {
                    baseline_rms = baselines["quiet.wav"]["rms_db"];
                    found = true;
                } else if (baselines.contains("rms_db")) {
                    baseline_rms = baselines["rms_db"];
                    found = true;
                }

                if (found) {
                    baseline_delta = rms_db - baseline_rms;
                    if (abs(baseline_delta) > 6.0) {
                        passed = false;
                    }
                }
            } catch(...) { }
        }
    }

    cout << "{" << endl;
    cout << "  \"rms_db\": " << rms_db << "," << endl;
    cout << "  \"peak_frequency_hz\": " << peak_freq << "," << endl;
    cout << "  \"baseline_delta_db\": " << baseline_delta << "," << endl;
    cout << "  \"passed_baseline\": " << (passed ? "true" : "false") << "," << endl;
    cout << "  \"crest_factor\": " << val_crest << "," << endl;
    cout << "  \"kurtosis\": " << val_kurt << endl;
    cout << "}" << endl;

    return 0;
}
