#include "SkeletonAnimation.h"

SkeletonAnimation::SkeletonAnimation(aiAnimation* animtion) {
	duration_ = animtion->mDuration;
	ticksPerSecond_ = std::max(animtion->mTicksPerSecond, 1.0);

	for (int i = 0; i < animtion->mNumChannels; i++) {
		aiNodeAnim* node = animtion->mChannels[i];
		BoneKeyFrame& keyFrame = boneAnimationNode_[node->mNodeName.C_Str()];
		for (int j = 0; j < node->mNumScalingKeys; j++) {
			keyFrame.scaling[node->mScalingKeys[j].mTime] = node->mScalingKeys[j].mValue;
		}
		for (int j = 0; j < node->mNumPositionKeys; j++) {
			keyFrame.translation[node->mPositionKeys[j].mTime] = node->mPositionKeys[j].mValue;
		}
		for (int j = 0; j < node->mNumRotationKeys; j++) {
			keyFrame.rotation[node->mRotationKeys[j].mTime] = node->mRotationKeys[j].mValue;
		}
	}
}

void SkeletonAnimation::createAnimationMatrixInternal(std::shared_ptr<SkeletonBoneNode> boneNode, std::vector<aiMatrix4x4>& matrixs, float time, aiMatrix4x4 parentMatrix)
{
	aiMatrix4x4 nodeMat = boneNode->localMatrix;
	auto animNode = boneAnimationNode_.find(boneNode->name);
	if (animNode != boneAnimationNode_.end()) {
		nodeMat = animNode->second.createMatrixByTimeMs(time);
	}
	aiMatrix4x4 GlobalTransformation = parentMatrix * nodeMat;

	matrixs[boneNode->boneIndex] = GlobalTransformation * boneNode->offsetMatrix;

	for (auto& it : boneNode->children) {
		createAnimationMatrixInternal(it, matrixs, time, GlobalTransformation);
	}
}

aiVector3D interp(const aiVector3D& start, const aiVector3D& end, float factor)
{
	return start + (end - start) * factor;
}

aiMatrix4x4 SkeletonAnimation::BoneKeyFrame::createMatrixByTimeMs(const float& timeMs)
{
	//移动插值
	aiVector3D vecTrans;
	auto endTranslation = translation.upper_bound(timeMs);
	auto startTranslation = endTranslation == translation.begin() ? endTranslation : --endTranslation;
	if (endTranslation != translation.end()) {
		float deltaTime = endTranslation->first - startTranslation->first;
		float factor = deltaTime == 0 ? 0 : (timeMs - startTranslation->first) / deltaTime;
		vecTrans = interp(startTranslation->second, endTranslation->second, factor);
	}
	else
		vecTrans = startTranslation->second;

	//旋转插值
	aiQuaternion quatRotation;
	auto endRotation = rotation.upper_bound(timeMs);
	auto startRotation = endRotation == rotation.begin() ? endRotation : --endRotation;
	if (endRotation != rotation.end()) {
		float deltaTime = endRotation->first - startRotation->first;
		float factor = deltaTime == 0 ? 0 : (timeMs - startRotation->first) / deltaTime;
		aiQuaternion::Interpolate(quatRotation, startRotation->second, endRotation->second, factor);
	}
	else
		quatRotation = startRotation->second;

	//缩放插值
	aiVector3D vecScale;
	auto endScaling = scaling.upper_bound(timeMs);
	auto startScaling = endScaling == scaling.begin() ? endScaling : --endScaling;
	if (endScaling != scaling.end()) {
		float deltaTime = endScaling->first - startScaling->first;
		float factor = deltaTime == 0 ? 0 : (timeMs - startScaling->first) / deltaTime;
		vecScale = interp(startTranslation->second, endTranslation->second, factor);
	}
	else
		vecScale = startTranslation->second;
	aiMatrix4x4 matrix;
	matrix.Decompose(vecScale, quatRotation, vecTrans);
	return matrix;
}