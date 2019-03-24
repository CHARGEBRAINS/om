#include "ani2d.hxx"

ani2d::ani2d()
{

}

void ani2d::draw(om::engine &e, float delta_time)
{
    current_time_ += delta_time;

    float one_frame_delta = 1.f / fps_;

    size_t how_may_frames_from_start = static_cast<size_t>(current_time_ / one_frame_delta);

    size_t current_frame_index = how_may_frames_from_start % sprites_.size();

    sprite& spr = sprites_.at(current_frame_index);

    spr.draw(e);
}
