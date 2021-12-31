#include "QFpsCamera.h"
#include <QMouseEvent>
#include <QDateTime>
#include <QCursor>
#include <QVulkanWindow>

QFpsCamera::~QFpsCamera()
{
	if (window_) {
		window_->removeEventFilter(this);
	}
}

void QFpsCamera::setup(QVulkanWindow* window)
{
	if (window) {
		window_ = window;
		window_->installEventFilter(this);
	}
}

QMatrix4x4 QFpsCamera::getMatrix() const
{
	return projection_ * view_;
}

bool QFpsCamera::eventFilter(QObject* watched, QEvent* event) {
	if (watched != nullptr && watched == window_) {
		switch (event->type())
		{
		case QEvent::Paint: {
			QCursor::setPos(window_->geometry().center());
			window_->requestActivate();
			projection_.setToIdentity();
			projection_.perspective(fov, window_->width() / (float)window_->height(), nearPlane, farPlane);
			window_->setCursor(Qt::BlankCursor);             //���������
			QCursor::setPos(window_->geometry().center());   //������ƶ���������
			break;
		}
		case QEvent::Resize: {
			projection_.setToIdentity();
			projection_.perspective(fov, window_->width() / (float)window_->height(), nearPlane, farPlane);
			break;
		}

		case QEvent::MouseMove: {
			if (window_->isActive() && window_->geometry().contains(QCursor::pos())) {
				QRect rect(0, 0, window_->width(), window_->height());
				QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
				float xoffset = mouseEvent->x() - rect.center().x();
				float yoffset = mouseEvent->y() - rect.center().y(); // ע���������෴�ģ���Ϊy�����Ǵӵײ����������������
				xoffset *= rotationSensitivity_;
				yoffset *= rotationSensitivity_;
				yaw_ += xoffset;
				pitch_ += yoffset;
				if (pitch_ > 1.55f)         //�����ӽ����Ƶ�[-89��,89��]��89��Լ����1.55
					pitch_ = 1.55f;
				if (pitch_ < -1.55f)
					pitch_ = -1.55f;
				cameraDirection_.setX(cos(yaw_) * cos(pitch_));
				cameraDirection_.setY(sin(pitch_));
				cameraDirection_.setZ(sin(yaw_) * cos(pitch_));
				view_.setToIdentity();
				view_.lookAt(cameraPos_, cameraPos_ + cameraDirection_, cameraUp_);
				QCursor::setPos(window_->geometry().center());
			}
			break;
		}
		case QEvent::KeyPress: {
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			keySet_.insert(keyEvent->key());
			if (keyEvent->key() == Qt::Key_Escape) {
				window_->close();
			}
			break;
		}
		case QEvent::KeyRelease: {
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			keySet_.remove(keyEvent->key());
			break;
		}
		case QEvent::UpdateRequest: {
			float time = QTime::currentTime().msecsSinceStartOfDay() / 1000.0;
			deltaTimeMs_ = time - lastFrameMs_;                           //�ڴ˴�����ʱ���
			lastFrameMs_ = time;
			if (!keySet_.isEmpty()) {
				float cameraSpeed = getMoveSpeed();
				if (keySet_.contains(Qt::Key_W))                           //ǰ
					cameraPos_ += cameraSpeed * cameraDirection_;
				if (keySet_.contains(Qt::Key_S))                           //��
					cameraPos_ -= cameraSpeed * cameraDirection_;
				if (keySet_.contains(Qt::Key_A))                           //��
					cameraPos_ -= QVector3D::crossProduct(cameraDirection_, cameraUp_) * cameraSpeed;
				if (keySet_.contains(Qt::Key_D))                           //��
					cameraPos_ += QVector3D::crossProduct(cameraDirection_, cameraUp_) * cameraSpeed;
				if (keySet_.contains(Qt::Key_Space))                       //�ϸ�
					cameraPos_.setY(cameraPos_.y() - cameraSpeed);
				if (keySet_.contains(Qt::Key_Shift))                       //�³�
					cameraPos_.setY(cameraPos_.y() + cameraSpeed);
				view_.setToIdentity();
				view_.lookAt(cameraPos_, cameraPos_ + cameraDirection_, cameraUp_);
			}
			break;
		}
		case QEvent::WindowActivate: {
			window_->setCursor(Qt::BlankCursor);             //���������
			QCursor::setPos(window_->geometry().center());   //������ƶ���������
			break;
		}
		case  QEvent::WindowDeactivate: {
			window_->setCursor(Qt::ArrowCursor);   //�ָ������
			keySet_.clear();
			break;
		}

		case  QEvent::Close: {
			keySet_.clear();
			break;
		}
		default:
			break;
		}
	}
	return QObject::eventFilter(watched, event);
}