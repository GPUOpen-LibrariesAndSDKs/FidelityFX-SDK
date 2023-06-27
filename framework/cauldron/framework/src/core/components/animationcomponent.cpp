// AMD Cauldron code
//
// Copyright(c) 2023 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "core/components/animationcomponent.h"
#include "core/entity.h"
#include "core/framework.h"

#include "misc/assert.h"
#include "misc/math.h"

#include<algorithm>

namespace cauldron
{
    const wchar_t*         AnimationComponentMgr::s_ComponentName     = L"AnimationComponent";
    AnimationComponentMgr* AnimationComponentMgr::s_pComponentManager = nullptr;

    AnimationComponentMgr::AnimationComponentMgr()
        : ComponentMgr()
    {
    }

    AnimationComponentMgr::~AnimationComponentMgr()
    {
    }

    AnimationComponent* AnimationComponentMgr::SpawnAnimationComponent(Entity* pOwner, ComponentData* pData)
    {
        // Create the component
        AnimationComponent* pComponent = new AnimationComponent(pOwner, pData, this);

        // Add it to the owner
        pOwner->AddComponent(pComponent);

        return pComponent;
    }

    void AnimationComponentMgr::Initialize()
    {
        CauldronAssert(ASSERT_CRITICAL,
                       !s_pComponentManager,
                       L"AnimationComponentMgr instance is non-null. Component managers can ONLY be created through framework registration using "
                       L"RegisterComponentManager<>()");

        // Initialize the convenience accessor to avoid having to do a map::find each time we want the manager
        s_pComponentManager = this;
    }

    void AnimationComponentMgr::Shutdown()
    {
        // Clear out the convenience instance pointer
        CauldronAssert(
            ASSERT_ERROR, s_pComponentManager, L"AnimationComponentMgr instance is null. Component managers can ONLY be destroyed through framework shutdown");
        s_pComponentManager = nullptr;
    }

    AnimationComponent::AnimationComponent(Entity* pOwner, ComponentData* pData, AnimationComponentMgr* pManager)
        : Component(pOwner, pData, pManager)
        , m_pData(reinterpret_cast<AnimationComponentData*>(pData))
    {
    }

    AnimationComponent::~AnimationComponent()
    {
    }

    void AnimationComponent::UpdateLocalMatrix(uint32_t animationIndex, float time)
    {
        if (animationIndex >= m_pData->m_pAnimRef->size())
            return;

        Animation* animation = (*m_pData->m_pAnimRef)[animationIndex];

        // Loop animation
        time = fmod(time, animation->GetDuration());

        const AnimChannel* pAnimChannel = animation->GetAnimationChannel(m_pData->m_nodeId);
        if (pAnimChannel->HasComponentSampler(AnimChannel::ComponentSampler::Translation) ||
            pAnimChannel->HasComponentSampler(AnimChannel::ComponentSampler::Rotation) ||
            pAnimChannel->HasComponentSampler(AnimChannel::ComponentSampler::Scale))
        {
            float frac, * pCurr, * pNext;

            // Animate translation
            Vec4 translation = Vec4(0, 0, 0, 0);
            if (pAnimChannel->HasComponentSampler(AnimChannel::ComponentSampler::Translation))
            {
                pAnimChannel->SampleAnimComponent(AnimChannel::ComponentSampler::Translation, time, &frac, &pCurr, &pNext);
                translation = ((1.0f - frac) * math::Vector4(pCurr[0], pCurr[1], pCurr[2], 0)) + ((frac)*math::Vector4(pNext[0], pNext[1], pNext[2], 0));
            }

            // Animate rotation
            Mat4 rotation = Mat4::identity();
            if (pAnimChannel->HasComponentSampler(AnimChannel::ComponentSampler::Rotation))
            {
                pAnimChannel->SampleAnimComponent(AnimChannel::ComponentSampler::Rotation, time, &frac, &pCurr, &pNext);
                rotation = math::Matrix4(math::slerp(frac, math::Quat(pCurr[0], pCurr[1], pCurr[2], pCurr[3]),
                                            math::Quat(pNext[0], pNext[1], pNext[2], pNext[3])),
                                            math::Vector3(0.0f, 0.0f, 0.0f));
            }

            // Animate scale
            Vec4 scale = Vec4(1, 1, 1, 1);
            if (pAnimChannel->HasComponentSampler(AnimChannel::ComponentSampler::Scale))
            {
                pAnimChannel->SampleAnimComponent(AnimChannel::ComponentSampler::Scale, time, &frac, &pCurr, &pNext);
                scale = ((1.0f - frac) * math::Vector4(pCurr[0], pCurr[1], pCurr[2], 0)) + ((frac)*math::Vector4(pNext[0], pNext[1], pNext[2], 0));
            }

            Mat4 transform = math::Matrix4::translation(translation.getXYZ()) * rotation * math::Matrix4::scale(scale.getXYZ());
            m_localTransform = transform;
        }
    }

    void cauldron::AnimationComponentMgr::UpdateComponents(double deltaTime)
    {
        static double time = 0.0;
        time += deltaTime;

        // Update local transforms
        for (auto& component : m_ManagedComponents)
        {
            component->Update(time);
        }

        // Update global transforms (process the hierarchy)
        for (auto& component : m_ManagedComponents)
        {
            const auto& parent = component->GetOwner()->GetParent();

            Mat4 parentTransform = parent == nullptr ? Mat4::identity() : parent->GetTransform();
            Mat4 globalTransform = parentTransform * static_cast<AnimationComponent*>(component)->GetLocalTransform();

            component->GetOwner()->SetPrevTransform(component->GetOwner()->GetTransform());
            component->GetOwner()->SetTransform(globalTransform);
        }
    }

    void AnimationComponent::Update(double time)
    {
        UpdateLocalMatrix(0, static_cast<float>(time));
    }

} // namespace cauldron
