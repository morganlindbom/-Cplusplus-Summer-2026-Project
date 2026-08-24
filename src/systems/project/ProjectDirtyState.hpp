// ProjectDirtyState.hpp
#pragma once

namespace pvd
{
/** Owns the lifecycle of unsaved persistent project changes. */
class ProjectDirtyState final
{
  public:
    /** Marks persistent project data as changed. */
    void markDirty()
    {
        dirty_ = true;
    }

    /** Records that a transactional save completed successfully. */
    void markSaved()
    {
        dirty_ = false;
    }

    /** Records that a complete project load completed successfully. */
    void markLoaded()
    {
        dirty_ = false;
    }

    /** Returns whether persistent project data has unsaved changes. */
    bool isDirty() const
    {
        return dirty_;
    }

  private:
    bool dirty_ = false;
};
} // namespace pvd
