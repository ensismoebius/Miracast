// SPDX-License-Identifier: GPL-2.0-or-later
/** @file Owning file-descriptor wrapper. */
#pragma once

namespace mc
{

/** RAII owner of a POSIX file descriptor. */
class FileDescriptor
{
public:
	/** Empty descriptor. */
	FileDescriptor() = default;
	/** Take ownership of fdnum. */
	explicit FileDescriptor(int fdnum) : fdnum_(fdnum)
	{
	}
	/** Close the descriptor. */
	~FileDescriptor();
	/** Move ownership. */
	FileDescriptor(FileDescriptor &&other) noexcept : fdnum_(other.release())
	{
	}
	/** Move-assign ownership. */
	FileDescriptor &operator=(FileDescriptor &&other) noexcept;
	/** Not copyable. */
	FileDescriptor(const FileDescriptor &) = delete;
	/** Not assignable by copy. */
	FileDescriptor &operator=(const FileDescriptor &) = delete;

	/** Raw descriptor number (-1 when empty). */
	int get() const
	{
		return fdnum_;
	}
	/** True when a descriptor is held. */
	bool valid() const
	{
		return fdnum_ >= 0;
	}
	/** Give up ownership without closing. */
	int release();
	/** Close the held descriptor and adopt fdnum. */
	void reset(int fdnum = -1);

private:
	int fdnum_ = -1;
};

} // namespace mc
