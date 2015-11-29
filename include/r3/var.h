/*
 *  var
 *
 */

/* 
 Copyright (c) 2010 Cass Everitt
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or
 without modification, are permitted provided that the following
 conditions are met:
 
 * Redistributions of source code must retain the above
 copyright notice, this list of conditions and the following
 disclaimer.
 
 * Redistributions in binary form must reproduce the above
 copyright notice, this list of conditions and the following
 disclaimer in the documentation and/or other materials
 provided with the distribution.
 
 * The names of contributors to this software may not be used
 to endorse or promote products derived from this software
 without specific prior written permission. 
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 POSSIBILITY OF SUCH DAMAGE. 
 
 
 Cass Everitt
 */

#ifndef __R3_VAR_H__
#define __R3_VAR_H__

#include "r3/atom.h"
#include "r3/linear.h"
#include <string>

namespace r3 {
	
	enum VarFlags {
		Var_ReadOnly = 0x01,
		Var_Archive = 0x02
	};
	
	
	class Var {
		Atom name;
		std::string desc;
		int flags;
	public:
		Var( const char * varName, const char * varDesc, int varFlags );
		Atom Name() const {
			return name;
		}
		const std::string & Description() const {
			return desc;
		}
		int Flags() const {
			return flags;
		}
		virtual std::string Get() const = 0;
		virtual void Set( const char * str ) = 0;
		
		// These are opt-in methods that mostly only make sense for numerical types.
		virtual void Incr( const char * incrBy, const char * upperBound = NULL) {}
		virtual void Decr( const char * decrBy, const char * lowerBound = NULL) {}
		virtual void Default() {}
	};

	class VarString : public Var {
		typedef std::string ValType;
		ValType val;
		ValType deflt;
	public:
		VarString( const char * varName, const char * varDesc, int varFlags, const ValType & varVal ) 
		: Var( varName, varDesc, varFlags ), val( varVal ), deflt( varVal ) {}
		virtual std::string Get() const;
		virtual void Set( const char * str );
		const ValType & GetVal() const { return val; }
		void SetVal( const ValType & varVal ) { val = varVal; }
		void Default() { val = deflt; }
		
	};
	
	class VarBool : public Var {
		typedef bool ValType;
		ValType val;
		ValType deflt;
	public:
		VarBool( const char * varName, const char * varDesc, int varFlags, const ValType & varVal ) 
		: Var( varName, varDesc, varFlags ), val( varVal ), deflt( varVal )  {}
		virtual std::string Get() const;
		virtual void Set( const char * str );
		const ValType & GetVal() const { return val; }
		void SetVal( const ValType & varVal ) { val = varVal; }
		void Default() { val = deflt; }
	};
	
	class VarInteger : public Var {
		typedef int ValType;
		ValType val;
		ValType deflt;
	public:
		VarInteger( const char * varName, const char * varDesc, int varFlags, const ValType & varVal ) 
		: Var( varName, varDesc, varFlags ), val( varVal ), deflt( varVal )  {}
		virtual std::string Get() const;
		virtual void Set( const char * str );
		const ValType & GetVal() const { return val; }
		void SetVal( const ValType & varVal ) { val = varVal; }
		virtual void Incr( const char * incrBy, const char * upperBound = NULL );
		virtual void Decr( const char * decrBy, const char * lowerBound = NULL );
		void Default() { val = deflt; }
	};
	
	class VarFloat : public Var {
		typedef float ValType;
		ValType val;
		ValType deflt;
	public:
		VarFloat( const char * varName, const char * varDesc, int varFlags, const ValType & varVal ) 
		: Var( varName, varDesc, varFlags ), val( varVal ), deflt( varVal )  {}
		virtual std::string Get() const;
		virtual void Set( const char * str );
		const ValType & GetVal() const { return val; }
		void SetVal( const ValType & varVal ) { val = varVal; }
		virtual void Incr( const char * incrBy, const char * upperBound = NULL );
		virtual void Decr( const char * decrBy, const char * lowerBound = NULL );
		void Default() { val = deflt; }
	};
	
	
	class VarVec2f : public Var {
		typedef Vec2f ValType;
		ValType val;
		ValType deflt;
	public:
		VarVec2f( const char * varName, const char * varDesc, int varFlags, const ValType & varVal )
		: Var( varName, varDesc, varFlags ), val( varVal ), deflt( varVal )  {}
		virtual std::string Get() const;
		virtual void Set( const char * str );
		const ValType & GetVal() const { return val; }
		void SetVal( const ValType & varVal ) { val = varVal; }
		void Default() { val = deflt; }
	};
	
	
	class VarVec3f : public Var {
		typedef Vec3f ValType;
		ValType val;
		ValType deflt;
	public:
		VarVec3f( const char * varName, const char * varDesc, int varFlags, const ValType & varVal )
		: Var( varName, varDesc, varFlags ), val( varVal ), deflt( varVal )  {}
		virtual std::string Get() const;
		virtual void Set( const char * str );
		const ValType & GetVal() const { return val; }
		void SetVal( const ValType & varVal ) { val = varVal; }
		void Default() { val = deflt; }
	};
	
	
	class VarVec4f : public Var {
		typedef Vec4f ValType;
		ValType val;
		ValType deflt;
	public:
		VarVec4f( const char * varName, const char * varDesc, int varFlags, const ValType & varVal )
		: Var( varName, varDesc, varFlags ), val( varVal ), deflt( varVal )  {}
		virtual std::string Get() const;
		virtual void Set( const char * str );
		const ValType & GetVal() const { return val; }
		void SetVal( const ValType & varVal ) { val = varVal; }
		void Default() { val = deflt; }
	};
	
	class VarRotationf : public Var {
		typedef Rotationf ValType;
		ValType val;
		ValType deflt;
	public:
		VarRotationf( const char * varName, const char * varDesc, int varFlags, const ValType & varVal )
		: Var( varName, varDesc, varFlags ), val( varVal ), deflt( varVal )  {}
		virtual std::string Get() const;
		virtual void Set( const char * str );
		const ValType & GetVal() const { return val; }
		void SetVal( const ValType & varVal ) { val = varVal; }
		void Default() { val = deflt; }
	};
	
	Var * FindVar( const char *varName );
	
}

#endif // __R3_VAR_H__