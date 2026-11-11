// SPDX-License-Identifier: GPL-2.0-or-later
#include "file_descriptor.hpp"

#include <unistd.h>

namespace mc
{

/** Close the descriptor. */
FileDescriptor::~FileDescriptor()
{
	reset();
}

/** Move-assign ownership. */
FileDescriptor &FileDescriptor::operator=(FileDescriptor &&other) noexcept
{
	if (this != &other)
		reset(other.release());
	return *this;
}

/** Give up ownership without closing. */
int FileDescriptor::release()
{
	const int held = fdnum_;
	fdnum_ = -1;
	return held;
}

/** Close the held descriptor and adopt fdnum. */
void FileDescriptor::reset(int fdnum)
{
	if (fdnum_ >= 0)
		::close(fdnum_);
	fdnum_ = fdnum;
}

} // namespace mc
