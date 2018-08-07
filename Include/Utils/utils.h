#ifndef __PARAMCHECK_H__
#define __PARAMCHECK_H__

#include <errno.h>

#define INVALID_FD(fd) (fd < 0)
#define INVALID_POINTER(p) (p == NULL)

// 禁止拷贝基类
class IUncopyable
{
	public:
		IUncopyable(){};
	private:
		IUncopyable(IUncopyable &);
		IUncopyable & operator=(const IUncopyable&);
};

// desc: 设置errno
// param: eno/需要设置的errno值
// return: void
inline void seterrno(int eno)
{
	errno = eno;
}

#endif
