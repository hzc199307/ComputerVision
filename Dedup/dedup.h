/*********************************************************************************
*     File Name           :     dedup.h
*     Created By          :     HeZhichao
*     Creation Date       :     [2016-05-27 12:50]
*     Last Modified       :     [2016-05-28 22:46]
*     Description         :     interface for video frame deduplicate
**********************************************************************************/

#ifndef SHENTU_DEDUP_DEDUP_H__
#define SHENTU_DEDUP_DEDUP_H__

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;

#include <map>
using namespace std;

#if (defined WIN32 || defined _WIN32 || defined WINCE) && defined SHENTU_EXPORTS
#undef SHENTU_EXPORTS
#define SHENTU_EXPORTS __declspec(dllexport)
#else
#undef SHENTU_EXPORTS
#define SHENTU_EXPORTS
#endif

extern "C"  {


  /// \brief Extract fingerprints from frame
  ///
  /// \param frame_stream jpeg or other image stream
  /// \param stream_len length of image stream
  /// \param fingerprint storage to save the extracted fingerprints, note this should be allocated outside the dll
  ///
  /// \return length of the extracted fingerprint
  SHENTU_EXPORTS int ExtractFrameFingerPrint(const char* frame_stream, int stream_len, char* fingerprint);

  SHENTU_EXPORTS int _ExtractFrameFingerPrint(Mat& frameMat, char* fingerprint);

  /// \brief Check if the frame is similar to previous frames
  ///
  /// \param video_id video id
  /// \param frame_id frame id
  /// \param fingerprint storage to save the extracted fingerprints
  /// \param fp_len length of the fingerprint
  ///
  /// \return frame id with same content with the input frame, if no same frame, return the input frame_id
  SHENTU_EXPORTS int DedupFrame(long long video_id, int frame_id, char* fingerprint, int fp_len);

  //SHENTU_EXPORTS bool isSimilar(Mat& fgMat1,Mat& fgMat2);
  SHENTU_EXPORTS bool isSimilar(double s);


  //SHENTU_EXPORTS int sum(int a,int b);//{return a+b;}

  SHENTU_EXPORTS double getSimilarity(char* fingerprint1, char* fingerprint2, int byteCount);

  float RANGES[] = {0, 256};
  class SHENTU_EXPORTS ColorHistogram {
  public:
    static double getColorHistSimilarity(Mat &matHist1, Mat &matHist2, int compare_method = CV_COMP_CORREL);
    static void colorBGRHistNum(Mat &matSrc, Mat &matBGRHistDst);
    static int getFingerprintLength() {return BINS * BINS * BINS * ROWS_BIN * COLS_BIN * 4;}
    static void char2Mat(const char *fingerprint, int fp_len, Mat &fingerprintMat);
    static void set(int BINS, int ROWS_BIN, int COLS_BIN, int DIMS, int DIVIDE_SIZE, int WIDTH, int HEIGHT, bool RESIZE);
  private:
    static int DIMS;
    static int BINS;
    static int ROWS_BIN;
    static int COLS_BIN;
    static int DIVIDE_SIZE;
    static int WIDTH;//150;
    static int HEIGHT;//260;
    static bool RESIZE;
    static void calcHist(Mat &matSrc, Mat &matBGRHistDst, int rowsBin, int colsBin);
    static void colorBGRHistNum11(Mat &matSrc, Mat &matBGRHistDst);
  };

  class SHENTU_EXPORTS Dedup {
  public:
    static const bool isPrint = false;
    static Dedup* getInstance();
    static Dedup* getNewInstance();
    /// \brief Check if the frame is similar to previous frames
    ///
    /// \param video_id video id
    /// \param frame_id frame id
    /// \param fingerprint storage to save the extracted fingerprints
    /// \param fp_len length of the fingerprint
    ///
    /// \return frame id with same content with the input frame, if no same frame, return the input frame_id
    int dedupFrame(long long video_id, int frame_id, char* fingerprint, int fp_len);
  private:
    static Dedup *instance;
    Dedup() {videosCount = 0;}
    map<long long, map<int, Mat> > videosMap;
    const int MaxVideoCount = 10000;
    const int MaxFramesCountPerVideo = 10;
    int videosCount;
    class CGarbo   //ËüµÄÎ¨Ò»¹¤×÷¾ÍÊÇÔÚÎö¹¹º¯ÊýÖÐÉ¾³ýCSingletonµÄÊµÀý
    {
    public:
      ~CGarbo() {
        if (Dedup::instance)
          delete Dedup::instance;
      }
    };
    static CGarbo Garbo;

  };
}


#endif
