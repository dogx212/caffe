//
// Created by Zengpan Fan on 8/3/16.
//

#ifndef CAFFE_MR_FEATURE_EXTRACTION_HPP
#define CAFFE_MR_FEATURE_EXTRACTION_HPP

#include <string>
#include "caffe/common.hpp"

class MRFeatureExtraction {
public:
  MRFeatureExtraction(): stop_signal_(false) { }

  template<typename Dtype>
  int feature_extraction_pipeline(
    std::string pretrained_binary_proto,
    std::string feature_extraction_proto,
    std::string extract_feature_blob_names,
    std::string target_pipe_path,
    int mini_batch_size,
    caffe::Caffe::Brew caffe_mode,
    int device_id
  );

  int run_feature_extraction_pipeline(
    const char* pretrained_binary_proto,
    const char* feature_extraction_proto
  );

  void stop_feature_extraction_pipeline() {
    stop_signal_ = true;
  }

  const char* get_input_pipe_path() {
    return "/tmp/foursquare_pcv1_input_pipe";
  }

  const char* get_output_pipe_path() {
    return "/tmp/foursquare_pcv1_output_pipe";
  }

  static const int get_batch_size() {
    return 50;
  }

  static const std::string get_share_memory_fs_path() {
    return std::string("/dev/shm");
  }

  static const std::string get_to_nn_batch_file_name_prefix() {
    return std::string("foursquare_pcv1_in_");
  }

  static const std::string get_from_nn_batch_file_name_prefix() {
    return std::string("foursquare_pcv1_out_");
  }

private:
  volatile bool stop_signal_;
};

#endif //CAFFE_MR_FEATURE_EXTRACTION_HPP
