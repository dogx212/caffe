// Copyright 2016 Foursquare Labs Inc. All Rights Reserved.

package com.foursquare.caffe.test

import caffe.Caffe.Datum
import collection.JavaConverters._
import com.foursquare.caffe.jMRFeatureExtraction
import com.google.protobuf.ByteString
import java.awt.image.{BufferedImage, DataBufferByte}
import java.io.{File, FileInputStream, FileOutputStream, PrintWriter}
import java.lang.{Runnable, Thread}
import javax.imageio.ImageIO
import scala.io.Source

object jMRFeatureExtractionTestApp extends App {
  val featureExtraction = jMRFeatureExtraction.Instance()
  val fileListPath = args(0)
  val source = Source.fromFile(fileListPath)
  val fileList = source.getLines
  val resultWriter = new PrintWriter(args.lift(1).getOrElse("jMRFeatureExtractionTestApp_result.txt"))

  featureExtraction.startAsync(
    args.lift(2).getOrElse("bvlc_googlenet.caffemodel"),
    args.lift(3).getOrElse("train_val.prototxt")
  )

  val worker = new Thread(new Runnable() {
    def run(): Unit = {
      var count = 0

      while(!Thread.interrupted) {
        try {
          println(s"Start reading $count")

          val featureDatum = featureExtraction.readDatum()

          count += 1

          featureDatum.getFloatDataList.asScala.foreach(d => resultWriter.print(s"$d "))
          resultWriter.println
        } catch {
          case e: InterruptedException => {
            println("Worker interrupted")
            resultWriter.close
          }
          case e: Exception => {
            println(s"Failed with exception [${e.getMessage}]")
            resultWriter.close
          }
        }
      }
    }
  })

  println("Start worker")
  worker.start()

  fileList.foreach(f => {
    val img = ImageIO.read(new File(f))
    val byteArray = img.getRaster.getDataBuffer.asInstanceOf[DataBufferByte].getData
    val datum = Datum.newBuilder
      .setData(ByteString.copyFrom(byteArray))
      .setHeight(img.getHeight)
      .setWidth(img.getWidth)
      .setChannels(3)
      .build()

    featureExtraction.writeDatum(datum)
  })

  source.close

  Thread.sleep(5000)

  // TODO(zen): add flush method to force sync.
  val ret = featureExtraction.stop()

  println(ret)

  resultWriter.close

  println("Result writer closed")

  worker.interrupt()

  println("Worker interrupted")

  System.exit(0)
}
