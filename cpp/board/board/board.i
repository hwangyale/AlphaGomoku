%module board
%{
#include "board.h"
%}

%include "std_vector.i"
namespace std
{
	%template(IntVector) vector<int>;
}

%include "board.h"