// $Id$

#ifndef MSXEXCEPTION_H
#define MSXEXCEPTION_H

#include <string>

namespace openmsx {

class MSXException
{
public:
	explicit MSXException(const std::string& message_)
		: message(message_) { }
	virtual ~MSXException() { }

	const std::string& getMessage() const {
		return message;
	}

private:
	const std::string message;
};

class FatalError
{
public:
	explicit FatalError(const std::string& message_)
		: message(message_) { }
	virtual ~FatalError() { }

	const std::string& getMessage() const {
		return message;
	}

private:
	const std::string message;
};

} // namespace openmsx

#endif // MSXEXCEPTION_H
