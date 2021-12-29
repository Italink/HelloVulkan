#ifndef QFPSCamera_h__
#define QFPSCamera_h__

#include <QObject>
#include <QVector3D>
#include <QSet>
#include <QMatrix4x4>

class QVulkanWindow;

class QFPSCamera :public QObject {
public:
	~QFPSCamera();
	void setup(QVulkanWindow* window);
	QMatrix4x4 getMatrix() const;
	float getMoveSpeed() const { return moveSpeed_; }
	void setMoveSpeed(float val) { moveSpeed_ = val; }
	float getRotationSensitivity() const { return rotationSensitivity_; }
	void setRotationSensitivity(float val) { rotationSensitivity_ = val; }
private:
	bool eventFilter(QObject* watched, QEvent* event) override;
private:
	float yaw_ = -M_PI_2;									    //偏航角
	float pitch_ = .0f;											//俯视角
	float rotationSensitivity_ = 0.0005f;							    //鼠标灵敏度
	float moveSpeed_ = 0.005f;									//控制移动速度
	QVector3D cameraPos_ = QVector3D(0.0f, 0.0f, 1.35f);        //摄像机初始位置
	QVector3D cameraDirection_ = QVector3D(cos(yaw_) * cos(pitch_), sin(pitch_), sin(yaw_) * cos(pitch_));  //摄像机方向
	QVector3D cameraRight_ = QVector3D::crossProduct({ 0.0f,1.0f,0.0f }, cameraDirection_);      //摄像机右向量
	QVector3D cameraUp_ = QVector3D::crossProduct(cameraDirection_, cameraRight_);         //摄像机上向量
	QSet<int> keySet_;			 //记录当前被按下按键的集合
	float deltaTimeMs_;			 //当前帧与上一帧的时间差
	float lastFrameMs_;			 //上一帧的时间
	QMatrix4x4 view_;			 //观察矩阵

	float fov = 45;
	float nearPlane = 0.001f;
	float farPlane = 10000.0f;
	QMatrix4x4 projection_;		 //投影矩阵
	QVulkanWindow* window_;
};

#endif // QFPSCamera_h__
