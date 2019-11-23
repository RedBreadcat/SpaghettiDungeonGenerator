#pragma once

class ConnectionNode
{
public:
	virtual ~ConnectionNode();	//Make the class polymorphic to satisfy requirements of dynamic_cast
	virtual bool IsRoom() const = 0;
	virtual bool IsCorridor() const = 0;
	bool operator==(const ConnectionNode& other) const;
};