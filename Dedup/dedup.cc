/*********************************************************************************
*     File Name           :     dedup.h
*     Created By          :     HeZhichao
*     Creation Date       :     [2016-05-27 12:50]
*     Last Modified       :     [2016-05-28 22:46]
*     Description         :     interface for video frame deduplicate
**********************************************************************************/
#include "dedup.h"
#include <shentu/io/cimage.h>

#include <pthread.h>

using namespace shentu::io;


int ExtractFrameFingerPrint(const char* frame_stream, int stream_len, char* fingerprint) {

	//load image from Byte stream
	CImage frameCImage;
	frameCImage.LoadFromStream(frame_stream, stream_len);
	Mat& frameMat = frameCImage.img();

	Mat fingerprintMat;
	//use 3x3 colorHistograms Methods
	ColorHistogram::colorBGRHistNum(frameMat, fingerprintMat);

	int byteCount = fingerprintMat.total() * fingerprintMat.elemSize1();
	char* fingerprintMatData = (char*)fingerprintMat.data;
	//copy data to fingerprint
	for (int i = 0; i < byteCount; i++, fingerprintMatData++, fingerprint++) {
		*fingerprint = *fingerprintMatData;
	}
	return byteCount;
}

int _ExtractFrameFingerPrint(Mat& frameMat, char* fingerprint) {

	Mat fingerprintMat;
	//use 3x3 colorHistograms Methods
	ColorHistogram::colorBGRHistNum(frameMat, fingerprintMat);

	int byteCount = fingerprintMat.total() * fingerprintMat.elemSize1();
	char* fingerprintMatData = (char*)fingerprintMat.data;
	//copy data to fingerprint
	for (int i = 0; i < byteCount; i++, fingerprintMatData++, fingerprint++) {
		*fingerprint = *fingerprintMatData;
	}
	return byteCount;
}

int DedupFrame(long long video_id, int frame_id, char* fingerprint, int fp_len) {
	//Mat fingerprintMat;
	//int sizes[] = {16,16,16,3,3};
	//fingerprintMat.create(5,sizes,5);
	Dedup::getInstance()->dedupFrame(video_id, frame_id, fingerprint, fp_len);
}

double getSimilarity(char* fingerprint1, char* fingerprint2, int byteCount) {
	Mat fingerprintMat1, fingerprintMat2;
	int sizes[] = {16, 16, 16, 3, 3};
	fingerprintMat1.create(5, sizes, 5);
	fingerprintMat2.create(5, sizes, 5);
	uchar* matData1 = fingerprintMat1.data;
	uchar* matData2 = fingerprintMat2.data;
	uchar* fp1 = (uchar*)fingerprint1;
	uchar* fp2 = (uchar*)fingerprint2;
	for (int i = 0; i < byteCount; i++, matData1++, fp1++) {
		*matData1 = *fp1;
	}
	for (int i = 0; i < byteCount; i++, matData2++, fp2++) {
		*matData2 = *fp2;
	}
	double s = ColorHistogram::getColorHistSimilarity(fingerprintMat1, fingerprintMat2);
	return s;
}

//////////////////////////////
//
//  Class Dedup start
//
pthread_mutex_t sum_mutex;//for thread
Dedup* Dedup::instance = NULL;
Dedup* Dedup::getInstance() {
	if (instance == NULL) {
		pthread_mutex_lock( &sum_mutex );
		if (instance == NULL) {
			instance = new Dedup();
		}
		pthread_mutex_unlock( &sum_mutex );
	}
	return instance;
}
Dedup* Dedup::getNewInstance() {
	if (instance != NULL) {
		pthread_mutex_lock( &sum_mutex );
		if (instance != NULL) {
			delete instance;
			instance = new Dedup();
		}
		pthread_mutex_unlock( &sum_mutex );
	}
	return instance;
}
int Dedup::dedupFrame(long long video_id, int frame_id, char* fingerprint, int fp_len) {
	int similar = frame_id;

	//fingerprint ： char to Mat
	Mat fgMat;
	ColorHistogram::char2Mat(fingerprint, fp_len, fgMat);

	if (isPrint)printf("\n## input frame, its Video_id = %lld, its frame_id = %d , its fingerprint = %d\n", video_id, frame_id, fp_len);
	//find video in Map
	map<long long, map<int, Mat> >::iterator itVideosMap = this->videosMap.find(video_id);
	if (itVideosMap == this->videosMap.end()) {
		//the first frame of this video
		map<int, Mat> framesMap;
		framesMap[frame_id] = fgMat;
		this->videosMap[video_id] = framesMap;
		if (isPrint)printf("Add video, its id = %lld\n", video_id);
		if (isPrint)printf("Add frame, its id = %d, its fingerprint length = %d\n", frame_id, fp_len);
	}
	else {
		//find video, compare with frames from the newest to the oldest
		map<int, Mat> &framesMap = this->videosMap[video_id];
		if (isPrint)printf("framesMap size = %d\n", (int)(framesMap.size()) );
		map<int, Mat>::reverse_iterator rit = framesMap.rbegin();
		for (rit; rit != framesMap.rend(); rit++)
		{
			double s = ColorHistogram::getColorHistSimilarity(rit->second, fgMat);
			if (isPrint)printf("frame similarty = %lf\n", s);
			if (isSimilar(s)) {
				similar = rit->first;
				if (isPrint)printf("Find similar frame, its id = %d , similarty = %lf\n", rit->first, s);
				break;
			}
		}
		if (rit == framesMap.rend()) {
			//no similar frame
			//if size equal to 'MaxFramesCountPerVideo' and 'oldest frame_id' less than 'input frame_id' , delete the oldest frame
			if (framesMap.size() >= MaxFramesCountPerVideo && framesMap.begin()->first < frame_id) {
				if (isPrint)printf("Delete frame, its id = %d\n", framesMap.begin()->first);
				framesMap.erase(framesMap.begin());
			}
			if (framesMap.size() < MaxFramesCountPerVideo) {
				//Add a frame
				framesMap[frame_id] = fgMat;
				if (isPrint)printf("Add frame, its id = %d, its fingerprint length = %d\t", frame_id, fp_len);
				if (isPrint)printf("framesMap size = %d\n", (int)(this->videosMap[video_id].size()) );
			}

		}
	}
	return similar;
}
// bool isSimilar(Mat& fgMat1,Mat& fgMat2){
// 	double s = ColorHistogram::getColorHistSimilarity(fgMat1,fgMat2);
// 	return s>=0.82552;//remain 60%
// }
bool isSimilar(double s) {
	return s >= 0.7; //0.82552;//remain 60%
}
// int Dedup::dedupFrame(long long video_id, int frame_id, char* fingerprint, int fp_len){
// 	int similar = frame_id;

// 	printf("\n## input frame, its Video_id = %lld, its frame_id = %d , its fingerprint = %d\n",video_id, frame_id,fp_len);
// 	//find video in Map
// 	map<long long, map<int,int> >::iterator itVideosMap= this->videosMap.find(video_id);
// 	if(itVideosMap == this->videosMap.end()) {
// 	//the first frame of this video
// 		map<int,int> framesMap;
// 		framesMap[frame_id]=fp_len;
// 		this->videosMap[video_id]=framesMap;
// 		printf("Add video, its id = %lld\n", video_id);
// 		printf("Add frame, its id = %d, its fingerprint = %d\n", frame_id,fp_len);
// 	}
// 	else{
// 	//find video, compare with frames from the newest to the oldest
// 		map<int,int> &framesMap = this->videosMap[video_id];
// 		printf("framesMap size = %d\n", (int)(framesMap.size()) );
// 		map<int,int>::reverse_iterator rit=framesMap.rbegin();
// 		for (rit;rit!=framesMap.rend();rit++)
// 		{
// 			//cout<<rit->first<<":"<<rit->second<<endl;
// 			if(rit->second==fp_len){
// 				similar = rit->first;
// 				printf("Find similar frame, its id = %d, its fingerprint = %d\n", rit->first,rit->second);
// 				break;
// 			}
// 		}
// 		if(rit==framesMap.rend()){
// 			//no similar frame
// 			//if size equal to 'MaxFramesCountPerVideo' and 'oldest frame_id' less than 'input frame_id' , delete the oldest frame
// 			if(framesMap.size()>=MaxFramesCountPerVideo && framesMap.begin()->first<frame_id){
// 				printf("Delete frame, its id = %d, its fingerprint = %d\n", framesMap.begin()->first,framesMap.begin()->second);
// 				framesMap.erase(framesMap.begin());
// 			}
// 			if(framesMap.size()<MaxFramesCountPerVideo){
// 				//Add a frame
// 				framesMap[frame_id]=fp_len;
// 				printf("Add frame, its id = %d, its fingerprint = %d\t", frame_id,fp_len);
// 				printf("framesMap size = %d\n", (int)(this->videosMap[video_id].size()) );
// 			}

// 		}
// 	}
// 	return similar;
// }
//
//  Class Dedup end
//
//////////////////////////////



//////////////////////////////
//
//  Class ColorHistogram start
//

int ColorHistogram::DIMS = 3;
int ColorHistogram::BINS = 4;
int ColorHistogram::ROWS_BIN = 1;
int ColorHistogram::COLS_BIN = 1;
int ColorHistogram::DIVIDE_SIZE = 10;
int ColorHistogram::WIDTH = 15;//150;
int ColorHistogram::HEIGHT = 26;//260;
bool ColorHistogram::RESIZE = true;
void ColorHistogram::set(int BINS, int ROWS_BIN, int COLS_BIN, int DIMS, int DIVIDE_SIZE, int WIDTH, int HEIGHT, bool RESIZE) {
	ColorHistogram::DIMS = DIMS;
	ColorHistogram::BINS = BINS;
	ColorHistogram::ROWS_BIN = ROWS_BIN;
	ColorHistogram::COLS_BIN = COLS_BIN;
	ColorHistogram::DIVIDE_SIZE = DIVIDE_SIZE;
	ColorHistogram::WIDTH = WIDTH;//150;
	ColorHistogram::HEIGHT = HEIGHT;//260;
	ColorHistogram::RESIZE = RESIZE;
	printf("BINS=%d ROWS_BIN=%d COLS_BIN=%d DIVIDE_SIZE=%d WIDTH=%d HEIGHT=%d\n", BINS, ROWS_BIN, COLS_BIN, DIVIDE_SIZE, WIDTH, HEIGHT);
}
void ColorHistogram::char2Mat(const char *fingerprint, int fp_len, Mat &fingerprintMat) {

	int sizes[] = {BINS, BINS, BINS, ROWS_BIN, COLS_BIN};
	fingerprintMat.create(DIMS, sizes, 5);
	const uchar* charData = (uchar*)fingerprint;
	uchar* matData = fingerprintMat.data;
	for (int i = 0; i < fp_len; i++, matData++, charData++) {
		*matData = *charData;
	}
}
/**将原图像matSrc转换为BGR直方图指纹matBGRHistDst**/
void ColorHistogram::colorBGRHistNum11(Mat &matSrc, Mat &matBGRHistDst) {

	//给三个通道设置取值范围
	const float* ranges [] = {RANGES, RANGES, RANGES};

	//给三个通道设置bin的个数
	int histSize[] = {BINS, BINS, BINS};

	//我们根据图像第0、1、2个通道，计算三维直方图
	int channels[] = {0, 1, 2};

	/*************************************************************************************************************************/
	/****                                    calcHist():计算图像块的直方图矩阵                                              ****/
	/****calcHist(),第1个参数为原数组区域列表；第二个参数为有计算几个原数组；参数3为需要统计的通道索引数；参数4为操作掩码              ****/
	/****第5个参数为存放目标直方图矩阵；参数6为需要计算的直方图的维数；参数7为每一维的bin的个数；参数8为每一维数值的取值范围            ****/
	/****参数10为每个bin的大小是否相同的标志，默认为1，即bin的大小都相同；参数11为直方图建立时清除内存痕迹标志，默认为0，即清除         ****/
	/************************************************************************************************************************/
	//resize之后再计算直方图 TODO
	static Mat matTmp;
	if (RESIZE) {
		cv::resize(matSrc, matTmp, cv::Size(WIDTH, HEIGHT), 0, 0, cv::INTER_CUBIC);
		cv::calcHist(&matTmp, 1, channels, Mat(), matBGRHistDst, 3, histSize, ranges, true, false);
	}
	else {
		cv::calcHist(&matSrc, 1, channels, Mat(), matBGRHistDst, 3, histSize, ranges, true, false);
	}

}

/**将原图像matSrc转换为BGR直方图指纹matBGRHistDst,并且把matSrc拆分成rowsBin*colsBin个子矩阵 **/
void ColorHistogram::colorBGRHistNum(Mat &matSrc, Mat &matBGRHistDst) {

	//printf("check BINS=%d ROWS_BIN=%d COLS_BIN=%d DIVIDE_SIZE=%d WIDTH=%d HEIGHT=%d\n", BINS, ROWS_BIN, COLS_BIN, DIVIDE_SIZE, WIDTH, HEIGHT);


	//两个至少有一个大于1 否则按照没有切分进行
	if ( (ROWS_BIN <= 1) && (COLS_BIN <= 1))
		return colorBGRHistNum11(matSrc, matBGRHistDst);

	/*************************************************************************************************************************/
	/****                                    calcHist():计算图像块的直方图矩阵                                              ****/
	/****calcHist(),第1个参数为原数组区域列表；第二个参数为有计算几个原数组；参数3为需要统计的通道索引数；参数4为操作掩码              ****/
	/****第5个参数为存放目标直方图矩阵；参数6为需要计算的直方图的维数；参数7为每一维的bin的个数；参数8为每一维数值的取值范围            ****/
	/****参数10为每个bin的大小是否相同的标志，默认为1，即bin的大小都相同；参数11为直方图建立时清除内存痕迹标志，默认为0，即清除         ****/
	/************************************************************************************************************************/
	//resize之后再计算直方图 TODO
	static Mat matTmp;
	if (RESIZE) {
		resize(matSrc, matTmp, cv::Size(WIDTH, HEIGHT), 0, 0, cv::INTER_CUBIC);
		ColorHistogram::calcHist(matTmp, matBGRHistDst, ROWS_BIN, COLS_BIN);
	}
	else {
		ColorHistogram::calcHist(matSrc, matBGRHistDst, ROWS_BIN, COLS_BIN);
	}

}
/**在opencv自带的calcHist基础上加了对图像矩阵的rowsBin*colsBin拆分**/
void ColorHistogram::calcHist(Mat &matSrc, Mat &matBGRHistDst, int rowsBin, int colsBin) {

	//printf("matSrc.rows: %d\tmatSrc.cols: %d\n",matSrc.rows,matSrc.cols);

	float ROW_RANGES [] = {0, (float)rowsBin};
	float COL_RANGES [] = {0, (float)colsBin};
	//给三个通道设置取值范围
	const float* ranges [] = {RANGES, RANGES, RANGES, ROW_RANGES, COL_RANGES}; //const float* ranges [] = {ROW_RANGES,COL_RANGES};//

	//给三个通道设置bin的个数
	int histSize[] = {BINS, BINS, BINS, rowsBin, colsBin}; //int histSize[] = {rowsBin,colsBin};//

	//我们根据图像第0、1、2个通道，计算三维直方图
	int channels[] = {0, 1, 2, 3, 4}; //int channels[] = {0,1};

	int rowsSrc = matSrc.rows;
	int colsSrc = matSrc.cols;
	int binWidth = colsSrc / colsBin;
	int binHeight = rowsSrc / rowsBin;

	//创建一个代表每一块区域所属位置的矩阵(例如rowsBin==3，colsBin==3；那么均分矩阵之后，左下角的子矩阵坐标是（2,2）)
	Mat submatLoc(rowsSrc, colsSrc, CV_8UC2);
	int recX, recY, recWidth, recHeight;
	//rowsBin*colsBin个掩码矩阵
	for (int i = 0; i < rowsBin; i++) {
		for (int j = 0; j < colsBin; j++) {

			recX = j * binWidth;
			recY = i * binHeight;
			recWidth = binWidth;
			recHeight = binHeight;
			if ( colsBin - j == 1 )
				recWidth = colsSrc - recX;
			if ( rowsBin - i == 1 )
				recHeight = rowsSrc - recY;

			//printf("loc(%d,%d)\tRect(%d,%d,%d,%d)\n",i,j,recX,recY,recWidth,recHeight);

			//对矩阵的选定区域赋予值
			Mat roi(submatLoc, Rect(recX, recY, recWidth, recHeight));
			roi = Scalar(i, j);

			//选定区域掩码为1
			Mat mask(rowsSrc, colsSrc, CV_8U, Scalar(0));
			Mat specified(mask, Rect(recX, recY, recWidth, recHeight));
			specified.setTo(1);

			Mat mixed[] = {matSrc, submatLoc};
			bool accumulate = i > 0 || j > 0; //第一次的时候才清除matBGRHistDst的内存，以后是累加
			cv::calcHist(mixed, 2, channels, mask, matBGRHistDst, DIMS, histSize, ranges, true, accumulate); //calcHist(&submatLoc,1,channels,mask,matBGRHistDst,2,histSize,ranges,true,accumulate);
		}
	}
	//cout<<"submatLoc:\n"<<submatLoc<<endl;
	//cout<<"\nmatBGRHistDst:\n"<<matBGRHistDst<<endl;
}
/*******************************************************************************************************************************/
/****                                        compareHist():比较2个直方图的相似度                                       		****/
/****        compareHist()，参数1为比较相似度的直方图1；参数2为比较相似度的直方图2；参数3为相似度的计算方式。有四种，            		****/
/****                  分别为CV_COMP_CORREL,CV_COMP_CHISQR,CV_COMP_INTERSECT,CV_COMP_BHATTACHARYYA                 		  	****/
/****		–   Correlation相关系数，相同为1，相似度范围为[ 1, 0 )													****/
/****		– CV_COMP_CHISQR Chi-Square卡方，相同为0，相似度范围为[ 0, +inf )													****/
/****		– CV_COMP_INTERSECT Intersection直方图交,数越大越相似，，相似度范围为[ 0, +inf )										****/
/****		– CV_COMP_BHATTACHARYYA Bhattacharyya distance做常态分别比对的Bhattacharyya 距离，相同为0,，相似度范围为[ 0, +inf )	****/
/*******************************************************************************************************************************/
double ColorHistogram::getColorHistSimilarity(Mat &matHist1, Mat &matHist2, int compare_method) {

	return cv::compareHist( matHist1, matHist2, compare_method );

}
//
//  Class ColorHistogram end
//
//////////////////////////////
