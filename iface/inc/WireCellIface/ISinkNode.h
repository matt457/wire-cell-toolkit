#ifndef WIRECELL_ISINKNODE
#define WIRECELL_ISINKNODE

#include "WireCellIface/INode.h"

#include <boost/any.hpp>
#include <vector>

namespace WireCell {

    /** A node which acts as a sink.
     */
    class ISinkNodeBase : public INode
    {
    public:
	typedef std::shared_ptr<ISinkNodeBase> pointer;

	virtual ~ISinkNodeBase() ;

	virtual NodeCategory category() {
	    return sinkNode;
	}

	virtual bool operator()(const boost::any& in) = 0;
    };

    template <typename InputType>
    class ISinkNode : public ISinkNodeBase
    {
    public:
	typedef InputType input_type;
	typedef ISinkNode<InputType> signature_type;
	typedef std::shared_ptr<const InputType> input_pointer;

	virtual ~ISinkNode() {}

	virtual bool operator()(const boost::any& anyin) {
	    const input_pointer& in = boost::any_cast<const input_pointer&>(anyin);
	    return (*this)(in);
	}

        virtual std::string signature() {
            return typeid(signature_type).name();
        }

	/// The calling signature:
	virtual bool operator()(const input_pointer& in) = 0;

	// Return the names of the types this node takes as input.
	virtual std::vector<std::string>  input_types() {
	    return std::vector<std::string>{typeid(input_type).name()};
	}

    };
    
}

#endif
