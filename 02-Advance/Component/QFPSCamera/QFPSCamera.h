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
	float yaw_ = -M_PI_2;									    //ƫ����
	float pitch_ = .0f;											//���ӽ�
	float rotationSensitivity_ = 0.0005f;							    //���������
	float moveSpeed_ = 0.005f;									//�����ƶ��ٶ�
	QVector3D cameraPos_ = QVector3D(0.0f, 0.0f, 1.35f);        //�������ʼλ��
	QVector3D cameraDirection_ = QVector3D(cos(yaw_) * cos(pitch_), sin(pitch_), sin(yaw_) * cos(pitch_));  //���������
	QVector3D cameraRight_ = QVector3D::crossProduct({ 0.0f,1.0f,0.0f }, cameraDirection_);      //�����������
	QVector3D cameraUp_ = QVector3D::crossProduct(cameraDirection_, cameraRight_);         //�����������
	QSet<int> keySet_;			 //��¼��ǰ�����°����ļ���
	float deltaTimeMs_;			 //��ǰ֡����һ֡��ʱ���
	float lastFrameMs_;			 //��һ֡��ʱ��
	QMatrix4x4 view_;			 //�۲����

	float fov = 45;
	float nearPlane = 0.001f;
	float farPlane = 10000.0f;
	QMatrix4x4 projection_;		 //ͶӰ����
	QVulkanWindow* window_;
};

#endif // QFPSCamera_h__
