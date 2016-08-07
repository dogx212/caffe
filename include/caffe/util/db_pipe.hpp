//
// Created by Zengpan Fan on 8/2/16.
//

#ifndef CAFFE_UTIL_DB_PIPE_HPP
#define CAFFE_UTIL_DB_PIPE_HPP

#include <atomic>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"

#include "caffe/util/db.hpp"
#include "caffe/proto/caffe.pb.h"

#include "mr_feature_extraction.hpp"

namespace db=caffe::db;

namespace caffe {
  namespace db {
    bool writeDelimitedTo(
        const google::protobuf::MessageLite& message,
        google::protobuf::io::ZeroCopyOutputStream* rawOutput
    );

    int readDelimitedFrom(
        google::protobuf::io::ZeroCopyInputStream* rawInput,
        google::protobuf::MessageLite* message
    );

    class PipeCursor : public Cursor {
    public:
      explicit PipeCursor(std::string& source): error_no_(1),
                                                file_name_(NULL),
                                                current_to_nn_batch_fd_(-1),
                                                current_to_nn_batch_stream_(NULL) {
        LOG(ERROR) << "Opening pipe " << source;
        input_fd_ = open(source.c_str(), O_RDWR);
        input_file_ = fdopen(input_fd_, "rw");

        open_to_nn_batch_stream();

        Next();
      }
      ~PipeCursor() {
        fclose(input_file_);
        close(input_fd_);

        if (file_name_ != NULL) {
          std::remove(file_name_);
          free(file_name_);
        }

        if (current_to_nn_batch_stream_ != NULL) {
          delete current_to_nn_batch_stream_;
        }

        close(current_to_nn_batch_fd_);
      }
      virtual void SeekToFirst() { } // TODO(zen): use ZeroCopyInputStream::BackUp
      virtual void Next();
      virtual std::string key() {
        ostringstream ss;
        ss << fake_key_;
        return ss.str();
      }
      virtual std::string value() {
        string out;
        current_.SerializeToString(&out);
        return out;
      }
      virtual bool valid() { return error_no_ >= 0; } // Next() will handle the recoverable errors like eof

    private:
      // Will delete the previous file
      void open_to_nn_batch_stream();

    private:
      static std::atomic<long> fake_key_;

    private:
      std::atomic<int> error_no_;
      int input_fd_;
      char* file_name_;
      FILE* input_file_;
      caffe::Datum current_;
      std::mutex current_to_nn_batch_stream_lock_;

      int current_to_nn_batch_fd_;
      google::protobuf::io::ZeroCopyInputStream *current_to_nn_batch_stream_;
    };

    class PipeTransaction : public Transaction {
    public:
      explicit PipeTransaction(std::string& source): current_from_nn_batch_id_(0),
                                                     current_from_nn_batch_fd_(-1) {
        out_fd_ = open(source.c_str(), O_RDWR);
      }
      virtual void Put(const std::string& key, const std::string& value) {
        caffe::Datum msg;
        msg.ParseFromString(value);
        Put(msg);
      }

      void Put(const caffe::Datum& value) {
        batch_.push(value);
      }

      virtual void Commit();
      ~PipeTransaction() {
        close(out_fd_);
      }

    private:
      int out_fd_;
      google::protobuf::io::ZeroCopyOutputStream* output_;
      std::queue<caffe::Datum> batch_;

      int current_from_nn_batch_id_;
      int current_from_nn_batch_fd_;

    DISABLE_COPY_AND_ASSIGN(PipeTransaction);
    };

    class Pipe : public DB {
    public:
      Pipe() {
        LOG(ERROR) << "Opening pipe db";
      }
      virtual ~Pipe() { Close(); }
      virtual void Open(const std::string& source, db::Mode mode) {
        source_ = source;
        mode_ = mode;
      }
      virtual void Close() { }
      virtual PipeCursor* NewCursor() {
        if (mode_ != db::READ) {
          std::ostringstream str_stream;
          str_stream << "Can only create cursor on read only pipe";
          throw std::runtime_error(str_stream.str());
        }

        return new PipeCursor(source_);
      }
      virtual PipeTransaction* NewTransaction() {
        if (mode_ != db::WRITE && mode_ != db::NEW) {
          std::ostringstream str_stream;
          str_stream << "Can only create transaction on write only pipe";
          throw std::runtime_error(str_stream.str());
        }

        return new PipeTransaction(source_);
      }

    private:
      std::string source_;
      db::Mode mode_;
    };
  }  // namespace db
}  // namespace caffe


#endif //CAFFE_UTIL_DB_PIPE_HPP
