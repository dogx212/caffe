package com.foursquare.caffe;

import caffe.*;
import com.google.protobuf.ByteString;
import java.io.*;
import java.lang.*;
import java.util.*;

public class jMRFeatureExtraction {
  private native int startFeatureExtraction(String pretrainedBinaryProto, String featureExtractionProto);

  private native void stopFeatureExtraction();

  private Thread featureExtractionThread = null;
  private volatile int featureExtractionReturnCode = -1;

  public native String getInputPipePath();
  public native String getOutputPipePath();

  public final static int batchSize = 50;

  public jMRFeatureExtraction() throws Exception { }

  public RandomAccessFile toNNFile = new RandomAccessFile(getInputPipePath(), "rw");
  public RandomAccessFile fromNNFile = new RandomAccessFile(getOutputPipePath(), "rw");

  private int currentToNNBatchId = 0;
  private int currentToNNBatchIndex = -1;
  private String currentToNNBatchFileNamePrefix = "/dev/shm/foursquare_pcv1_in_";
  private PrintWriter currentToNNBatchFileStream =
    new PrintWriter(currentToNNBatchFileNamePrefix + currentToNNBatchId);

  public void writeDatum(Caffe.Datum datum) throws Exception {
    if (currentToNNBatchIndex == batchSize - 1) {
      currentToNNBatchFileStream.close();
      toNNFile.write((currentToNNBatchFileNamePrefix + currentToNNBatchId + "\n").getBytes());

      currentToNNBatchIndex = -1;
      ++currentToNNBatchId;

      currentToNNBatchFileStream = new PrintWriter(currentToNNBatchFileNamePrefix + currentToNNBatchId);
    }
    currentToNNBatchFileStream.println(datum.toByteString().toStringUtf8());
    ++currentToNNBatchIndex;
  }

  private int currentFromNNBatchIndex = -1;
  private BufferedReader currentFromNNBatchFileStream = null;

  public Caffe.Datum readDatum() throws Exception {
    if (currentFromNNBatchFileStream == null || currentFromNNBatchIndex == batchSize - 1) {
      if (currentFromNNBatchFileStream != null) {
        currentFromNNBatchFileStream.close();
      }

      currentFromNNBatchIndex = -1;

      String fileName = fromNNFile.readLine();

      currentFromNNBatchFileStream = new BufferedReader(new FileReader(fileName));
    }

    String line = currentFromNNBatchFileStream.readLine();
    Caffe.Datum datum = Caffe.Datum.parseFrom(ByteString.copyFromUtf8(line));

    if (datum == null) {
      currentFromNNBatchIndex = batchSize - 1;
    } else {
      ++currentFromNNBatchIndex;
    }

    return datum;
  }

  public void start(String pretrainedBinaryProto, String featureExtractionProto) {
    featureExtractionThread = new Thread(new Runnable() {
      public void run() {
        featureExtractionReturnCode = startFeatureExtraction(pretrainedBinaryProto, featureExtractionProto);
      }
    });

    featureExtractionThread.start();
  }

  public int stop() {
    stopFeatureExtraction();

    try {
      featureExtractionThread.join(5000);
    } catch(Exception e) {
      return -1;
    }

    return featureExtractionReturnCode;
  }

  static {
    File jar = new File(jMRFeatureExtraction.class.getProtectionDomain().getCodeSource().getLocation().getPath());
    System.load(jar.getParentFile().toURI().resolve("libcaffe_jni.so").getPath());
  }
}
