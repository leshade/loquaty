
#ifndef	__LOQUATY_CODE_BUFFER_H__
#define	__LOQUATY_CODE_BUFFER_H__	1

namespace	Loquaty
{
	//////////////////////////////////////////////////////////////////////////
	// 実行コード・バッファ
	//////////////////////////////////////////////////////////////////////////

	class	LCodeBuffer	: public Object
	{
	public:
		// 命令疑似コード表記；
		// sp, ap, dp, fp, xp, yp : 各種スタック参照ポインタ（スタック指標）
		// ip		  : 命令ポインタ
		// imm        : 32bit 符号あり整数即値
		// sop1, sop2 : 命令オペランド（st への指標 8bit 即値）
		// op3        : 命令オペランド（8bit 即値）
		// imop       : 64bit 即値オペランド（数値又はオブジェクト／オブジェクトの場合 Ref 操作はしない）
		// imop.i     : 64bit 整数即値
		// imop.f     : 64bit 浮動小数点即値
		// imop.cls   : LClass* 即値（ReleaseRef 操作はしない）
		// imop.fnc   : LFunctionObj* 即値（ReleaseRef 操作はしない）
		// imop.op1   : PFN_OPERATOR1 即値
		// imop.op2   : PFN_OPERATOR2 即値
		// imop.obj   : LObject* 即値（ReleaseRef 操作はしない）
		// st[index]  : スタック要素
		// fp[index]  : st[fp + index]
		// sp[index]  : st[sp - 1 - index]
		// xp[index]  : st[xp + index]
		// sync(x)    : synchronized(x)
		// push(x)    : st[sp]:=x, sp+=1
		// pop()      : sp-=1, st[sp]
		// addref(x)  : LObject::AddRef((LObject*)x)
		// free(x)    : LObject::ReleaseRef((LObject*)x)
		// freefor(x) : while(yp>=x){ obj:=st[yp], yp:=st[yp+1], free(obj) }, dp:=min(dp,x)
		// searchxp() : while(xp!=-1&&xp[0]==-1){ xp:=xp[4] }, if(xp==-1){ unhandled_exception(); }
		// cast(cls,x): ((LObject*)x)->CastClassTo(cls)
		// alloc(p,n) : ((LPointerObj*)p)->SetPointer( new LArrayBufStorage(n), 0, n )
		//              ※バッファの内容はクラス情報を使って初期化する
		// addr(p,i,n): ((LObject*)p)->GetBufferPoiner()->GetPointer(i,n)
		//              ※ align でアライメントチェックし、例外エラーを発生させる
		// addrchk(p) : ポインタの評価値のアライメントチェック
		//              ※ align でアライメントチェックし、例外エラーを発生させる
		// eximm      : 拡張オペランド（次の命令に 64bit 即値オペランドを追加する）
		// ret_val    : 関数返り値 (※返却前に AddRef し、受け取り側が ReleaseRef する)
		// ret_type   : 関数返り値型 (※LType::Primitive)
		// code_buf   : 実行コードバッファ・ポインタ
		// thrown     : 送出された例外オブジェクト（未キャッチ）
		// except     : 例外オブジェクト
		// sync_locker: オブジェクト同期保持オブジェクト（解放すると symchronized から抜ける）
		//              ※symchronized 命令が実行され、実際に排他処理権が獲得されると
		//                同期オブジェクトが生成されスタックにプッシュされる。
		//                この同期オブジェクトが ReleaseRef されると排他処理券は解放される。
		//
		// ※ret_val, thrown, except
		//   これらは LValue, LObjPtr で保持するので
		//   代入操作は以前のオブジェクトを自動的に ReleaseRef する

		// ABI  : 関数呼び出し
		// stack: ap[0]:arg1, ap[1]:arg2, ..., ret_ip, ret_code_buf
		//
		//		関数引数は引数順に push する。
		//		戻りアドレスは ip, code_buf の順で push する。
		//		簡単のため引数の先頭 sp の位置を ap に設定してから呼び出す。
		//		引数にオブジェクトを渡す場合には呼び出し前に AddRef し、
		//		解放チェーンに追加された状態にしておく。
		//		※引数受け渡し中に例外などが発生しても解放漏れが発生しないようにするため

		// ABI  : オブジェクト（解放）チェーン
		// stack: yp[0]:object, yp[1]:prev_yp
		//
		//		スタック上の yp が指す位置に解放すべきオブジェクトを格納する。
		//		yp[1] に前のリスト位置をポイントすることによって
		//		スタック上の指定の位置までオブジェクトを解放することができる。
		//		（sp の移動に伴って yp>=sp の区間のオブジェクトを解放できる）
		//		yp が -1 の時には解放すべきオブジェクトは存在しない。

		struct	ReleaserDesc
		{
			LValue::Primitive	object ;
			LValue::Primitive	prev_fp ;
		} ;

		// ABI  : 構造化例外
		// stack: xp[0]:except_handler, xp[1]:finally_handler,
		//		  xp[2]:code_buf, xp[3]:fp, xp[4]:prev_xp
		//
		//		スタック上の xp が指す位置に例外ハンドラの情報を保存する。
		//		ハンドラのアドレスが -1 の場合、ハンドラは存在しない。
		//		末尾（xp[4]）に以前の xp を保存する。
		//		xp が -1 の時には例外ハンドラ・チェーンが存在しない。
		//
		//		try 句の開始で codeEnterTry 命令を実行し、
		//		try の末尾で codeCallFinally 命令を実行する。
		//		finally 句の末端では codeRetFinally 命令を実行するが、
		//		途中 break 等で離脱する場合は codeLeaveFinally 命令を実行する。
		//
		// CallFinally:
		//		if ( xp[1] != -1 ) {
		//			freefor(xp+5)
		//			sp = xp+5
		//			push(thrown.detach()), push(ret_val), push(ret_type)
		//			push(ip), push(code_buf)
		//			code_buf:=xp[2], ip:=xp[1]
		//			xp[0]:=-1, xp[1]:=-1, fp:=xp[3]
		//		} else {
		//			LeaveTry
		//		}
		//
		// RetFinally:
		//		code_buf:=pop(), ip:=pop()
		//		ret_type:=pop(), ret_val:=pop(), thrown:=pop()
		//		LeaveTry
		//		if ( thrown != null ) {
		//			push( thrown.detach() )
		//			Throw
		//		}
		//
		// LeaveFinally:
		//		pop(), pop()
		//		ret_type:=pop(), ret_val:=pop(), free(pop())
		//		LeaveTry
		//
		// LeaveTry:
		//		freefor(xp), sp:=xp, fp:=xp[3], xp:=xp[4]
		//		thrown:=null
		//
		// Throw:
		//		thrown:=pop()	; ※AddRef されたオブジェクト
		//		while ( xp != -1 ) {
		//			if ( xp[0] == -1 ) {
		//				if ( xp[1] != -1 ) {
		//					ip -= 1
		//					CallFinally		; RetFinally すると再び Throw が実行される
		//				}
		//				xp:=xp[4]
		//			}
		//			else {
		//				break
		//			}
		//		}
		//		except:=thrown
		//		thrown:=null
		//		if ( xp != -1 ) {
		//			code_buf:=xp[2], ip:=xp[0], fp:=xp[3]
		//			xp[0]:=-1
		//			freefor(xp+5)
		//			sp = xp+5
		//		} else {
		//			unhandled_exception();
		//		}

		struct	ExceptionDesc
		{
			LValue::Primitive	cpHandler ;
			LValue::Primitive	cpFinally ;
			LValue::Primitive	codeBuf ;
			LValue::Primitive	fp ;
			LValue::Primitive	prev_xp ;
		} ;

		static constexpr const size_t	ExceptDescSize		= 5 ;
		static constexpr const size_t	ExceptDescHandler	= 0 ;
		static constexpr const size_t	ExceptDescFinally	= 1 ;
		static constexpr const size_t	ExceptDescCodeBuf	= 2 ;
		static constexpr const size_t	ExceptDescFP		= 3 ;
		static constexpr const size_t	ExceptDescPrevXP	= 4 ;

		static constexpr const ssize_t	InvalidCodePos		= -1 ;
		static constexpr const ssize_t	ExceptNoHandler		= InvalidCodePos ;

		struct	FinallyRetStack
		{
			LValue::Primitive	saveThrew ;
			LValue::Primitive	saveRetValue ;
			LValue::Primitive	saveRetType ;
			LValue::Primitive	ipRet ;
			LValue::Primitive	codeBuf ;
		} ;
		static constexpr const size_t	FinallyRetStackSize	= 5 ;


		// 命令コード
		enum	InstructionCode
		{
			codeNoOperation,
			codeEnterFunc,			// push(dp), push(fp), fp:=sp, dp:=sp, sp+=imm   ; ※拡張された領域は 0 クリアされる
			codeLeaveFunc,			// freefor(fp), sp:=fp, fp:=pop(), dp:=pop()
			codeEnterTry,			// push(code_buf), push(fp), push(xp), xp:=sp-5
			codeCallFinally,		// see CallFinally
			codeRetFinally,			// see RetFinally
			codeLeaveFinally,		// see LeaveFinally
			codeLeaveTry,			// see LeaveTry
			codeThrow,				// see Throw
			codeAllocStack,			// sp+=imm, dp:=sp   ; ※拡張された領域は 0 クリアされる
			codeFreeStack,			// freefor(sp-imm), sp-=imm
			codeFreeStackLocal,		// freefor(fp+imop.i), sp:=fp+imop.i
			codeFreeLocalObj,		// free(fp[imop.i]), fp[imop.i]:=null
			codePushObjChain,		// push(yp), yp:=sp - 2
			codeMakeObjChain,		// fp[imop.i]:=yp, yp:=fp + imop.i - 1
			codeSynchronize,		// sync(fp[imop.i]), push(sync_locker), push(yp), yp:=sp - 2
			codeReturn,				// code_buf:=pop(), ip:=pop(), freefor(sp-imm), sp-=imm
			codeSetRetValue,		// ret_val:=sp[sop1], type=op3, freefor(sp-imm), sp-=imm  ; ※ op3:LType::Primitive
			codeMoveAP,				// ap:=sp - imm
			codeCall,				// func:=imop.fnc, push(ip), push(code_buf), call func...
			codeCallVirtual,		// class:=imop.cls, vidx:=imm, push(ip), push(code_buf), call class[vidx]...
			codeCallIndirect,		// func:=pop(), push(ip), push(code_buf), call func...
			codeExImmPrefix,		// eximm:=imop
			codeClearException,		// except:=null
			codeLoadRetValue,		// addref(ret_val), push(ret_val)
			codeLoadException,		// push(except)			; ※ ReleaseRef してはならない
			codeLoadArg,			// push(st[ap+imm])
			codeLoadImm,			// push(imop),  ; ※ op3:LType::Primitive
			codeLoadLocal,			// type:=op3, push((type)fp[imop.i])
			codeLoadLocalOffset,	// type:=op3, push((type)fp[imop.i + sp[sop1]])
			codeLoadStack,			// push(sp[sop1])
			codeLoadFetchAddr,		// align:=op3, push(addr(sp[sop1], imop.i, imm))   ; align でアライメントチェック, align==0:nullptrを許可
			codeLoadFetchAddrOffset,// align:=op3, push(addr(sp[sop1], sp[sop2] + imop.i, imm))   ; align でアライメントチェック, align==0:nullptrを許可
			codeLoadFetchLAddr,		// align:=op3, push(addr(fp[sop1], sp[sop2] + imop.i, imm))   ; align でアライメントチェック, align==0:nullptrを許可
			codeCheckLPtrAlign,		// align:=op3, addrchk(fp[imop.i]+sp[sop1]+imm)
			codeCheckPtrAlign,		// align:=op3, addrchk(sp[sop1]+imm)
			codeLoadLPtr,			// type:=op3, push((type)[fp[imop.i]+imm])  ; ※fp[imop.i]:Object型, op3:LType::Primitive
			codeLoadLPtrOffset,		// type:=op3, push((type)[fp[imop.i]+sp[sop1]+imm)  ; ※fp[imop.i]:Object型, sp[sop1]:int型, op3:LType::Primitive
			codeLoadByPtr,			// type:=op3, push((type)[st[sop1]+imm])  ; ※st[sop1]:Pointer型, op3:LType::Primitive
			codeLoadByLAddr,		// type:=op3, push((type)[fp[imop.i]+imm])  ; ※fp[imop.i]:直接アドレス, op3:LType::Primitive
			codeLoadByAddr,			// type:=op3, push((type)[sp[sop1]+imm])  ; ※sp[sop1]:直接アドレス, op3:LType::Primitive
			codeLoadLocalPtr,		// push(new Pointer(&st[fp+imop.i], imm))  ; ※後で free() が必要
			codeLoadObjectPtr,		// obj:=sp[sop1], push(obj.getbuf())       ; ※後で free() が必要
			codeLoadLObjectPtr,		// obj:=fp[imop.i], push(obj.getbuf())     ; ※後で free() が必要
			codeLoadObject,			// push(clone imop.obj)  ; ※後で free() が必要
			codeNewObject,			// class:=imop.cls, push(new class)  ; ※後で free() が必要
			codeAllocBuf,			// class:=imop.cls, alloc(sp[sop1],sp[sop2])  ; class が構造体の場合初期化する
			codeRefObject,			// addref(sp[sop1])
			codeFreeObject,			// free(sp[sop1])
			codeMove,				// sp[sop2]:=sp[sop1], freefor(sp-imm), sp-=imm
			codeStoreLocalImm,		// type:=op3, (type)fp[imm]:=imop
			codeStoreLocal,			// type:=op3, (type)fp[imop.i]:=sp[sop1], freefor(sp-imm), sp-=imm
			codeExchangeLocal,		// type:=op3, temp:=fp[imop.i], fp[imop.i]:=sp[sop1], sp[sop1]:=temp, freefor(sp-imm), sp-=imm
			codeStoreByPtr,			// type:=op3, (type)[sp[sop1]+imm]:=sp[sop2]  ; ※st[sop1]:Pointer型, op3:LType::Primitive
			codeStoreByLPtr,		// type:=op3, (type)[fp[imop.i]+imm]:=sp[sop2]  ; ※fp[imop.i]:Pointer型, op3:LType::Primitive
			codeStoreByLAddr,		// type:=op3, (type)[fp[imop.i]+imm]:=sp[sop2]  ; ※fp[imop.i]:直接アドレス, op3:LType::Primitive
			codeStoreByAddr,		// type:=op3, (type)[sp[sop1]+imm]:=sp[sop2]  ; ※st[sop1]:直接アドレス, op3:LType::Primitive
			codeBinaryOperate,		// res:=imop.op2(sp[sop1],sp[sop2],eximm), eximm:=null, freefor(sp-imm), sp-=imm, push(res)  ; ※ op3:Symbol::OperatorIndex
			codeUnaryOperate,		// res:=imop.op1(sp[sop1],eximm), eximm:=null, freefor(sp-imm), sp-=imm, push(res)  ; ※ op3:Symbol::OperatorIndex
			codeCastObject,			// class:=imop.cls, obj:=st[sop1], freefor(sp-imm), sp-=imm, push(cast(class,obj))  ; ※後で free() が必要
			codeJump,				// ip:=imop.i, freefor(sp-imm), sp-=imm
			codeJumpConditional,	// n:=imm, if(sp[sop1]){ip:=imop.i, n+=sop2+op3*256}, freefor(sp-n), sp-=n
			codeJumpNonConditional,	// n:=imm, if(!sp[sop1]){ip:=imop.i, n+= sop2+op3*256}, freefor(sp-n), sp-=n
			codeGetElement,			// index:=st[sop2], obj:=st[sop1], freefor(sp-imm), sp-=imm, push(obj.get(index))
			codeGetElementAs,		// str:=st[sop2], obj:=st[sop1], freefor(sp-imm), sp-=imm, push(obj.get(str))
			codeGetElementName,		// index:=st[sop2], obj:=st[sop1], freefor(sp-imm), sp-=imm, push(obj.getname(index))
			codeSetElement,			// obj2:=st[op3], index:=st[sop2], obj1:=st[sop1], addref(obj2), free(obj1.set(index,obj2)), freefor(sp-imm), sp-=imm
			codeSetElementAs,		// obj2:=st[op3], str:=st[sop2], obj1:=st[sop1], addref(obj2), free(obj1.set(str,obj2)), freefor(sp-imm), sp-=imm
			codeObjectToInt,		// obj:=sp[sop1], sp[sop2]:=long(obj)
			codeObjectToFloat,		// obj:=sp[sop1], sp[sop2]:=double(obj)
			codeObjectToString,		// obj:=sp[sop1], push(obj.toString())  ; ※後で free() が必要
			codeIntToObject,		// int_val:=sp[sop1], push(new Integer(int_val))  ; ※後で free() が必要
			codeFloatToObject,		// float_val:=sp[sop1], push(new Double(float_val))  ; ※後で free() が必要
			codeIntToFloat,			// int_val:=sp[sop1], sp[sop2]:=double(int_val)
			codeUintToFloat,		// uint_val:=sp[sop1], sp[sop2]:=double(uint_val)
			codeFloatToInt,			// float_val:=sp[sop1], sp[sop2]:=long(float_val)
			codeFloatToUint,		// float_val:=sp[sop1], sp[sop2]:=long(float_val)

			codeInstructionCount,
		} ;

		// 副作用のないロード（プッシュ）命令か？
		static bool IsLoadPushCodeWithoutEffects( InstructionCode code ) ;

		// 即値オペランド
		union	ImmediateOperand
		{
			LValue::Primitive		value ;
			Symbol::PFN_OPERATOR1	pfnOp1 ;
			Symbol::PFN_OPERATOR2	pfnOp2 ;
			LClass *				pClass ;
			LFunctionObj *			pFunc ;
			void *					pExtra ;
		} ;

		// 命令語
		struct	Word
		{
			std::uint8_t		code ;		// enum InstructionCode
			std::uint8_t		sop1 ;		// stack operand index
			std::uint8_t		sop2 ;
			std::uint8_t		op3 ;
			std::int32_t		imm ;		// 32bit immediate value
			ImmediateOperand	imop ;		// 64bit immediate value
		} ;

		// 命令実行関数プロトタイプ
		typedef void (LContext::*PFN_Instruction)( const Word& word ) ;

		// デバッグ情報
		class	DebugSourceInfo
		{
		public:
			size_t	m_iCodeFirst ;		// 対応するコードのインデックス
			size_t	m_iCodeEnd ;
			size_t	m_iSrcFirst ;		// ソースコード上の文字インデックス
			size_t	m_iSrcEnd ;
		} ;

		class	DebugLocalVarInfo
		{
		public:
			LLocalVarArrayPtr	m_varInfo ;
			size_t				m_iCodeFirst ;
			size_t				m_iCodeEnd ;
		} ;

	protected:
		std::vector<Word>				m_buffer ;
		std::vector<PFN_Instruction>	m_instmap ;
		std::vector<LObjPtr>			m_objPool ;

		LFunctionObj *					m_pFunction ;
		LSourceFilePtr					m_pSourceFile ;
		std::vector<DebugSourceInfo>	m_dbgSrcInfos ;
		std::vector<DebugLocalVarInfo>	m_dbgVarInfos ;

		friend class LCompiler ;
		friend class LContext ;

	public:
		LCodeBuffer( void )
			: m_pFunction( nullptr )
		{
		}

		// コードバッファ
		const Word& GetCodeAt( size_t ip ) const
		{
			assert( ip < m_buffer.size() ) ;
			return	m_buffer.at(ip) ;
		}
		Word& CodeAt( size_t ip )
		{
			assert( ip < m_buffer.size() ) ;
			return	m_buffer.at(ip) ;
		}
		size_t GetCodeSize( void ) const
		{
			return	m_buffer.size() ;
		}
		const std::vector<Word>& GetBuffer( void ) const
		{
			return	m_buffer ;
		}
		void AddCode( const Word& word ) ;
		void InsertCode( size_t iPos, const Word& word ) ;

		// オブジェクトプール
		const LObjPtr& ObjectAt( size_t index ) const
		{
			assert( index < m_objPool.size() ) ;
			return	m_objPool.at(index) ;
		}
		size_t AddObjectToPool( const LObjPtr& pObj )
		{
			size_t	index = m_objPool.size() ;
			m_objPool.push_back( pObj ) ;
			return	index ;
		}

		// デバッグ情報
		void AttachFunction( LFunctionObj * pFunc ) ;
		LFunctionObj * GetFunction( void ) const ;

		void AttachSourceFile( LSourceFilePtr pSourceFile ) ;
		LSourceFilePtr GetSourceFile( void ) const ;

		void AddDebugSourceInfo
			( const DebugSourceInfo& dbSrcInf ) ;
		const DebugSourceInfo* FindDebugSourceInfo( size_t iCodePos ) const ;
		const DebugSourceInfo* FindDebugSourceInfoAtSource( size_t iSrcPos ) const ;

		void AddDebugLocalVarInfo
			( LLocalVarArrayPtr pVar, size_t iFirst, size_t iEnd ) ;
		void GetDebugLocalVarInfos
			( std::vector<DebugLocalVarInfo>& dbgVarInfos, size_t iPos ) const ;

	public:
		// コードを疑似ニーモニックに変換
		LString MnemonicOf( const Word& word, size_t iCodePos ) const ;
		static LString FormatMnemonic( const wchar_t * mnemonic, const Word& word ) ;

	public:
		enum	MnemonicFlag
		{
			optionSubSp		= 0x0001,		// pop(imm)
			optionSubSp2	= 0x0002,		// cpop(sop2+op3*0x100)
			optionApImm		= 0x0004,		// ap[imm]
			optionFpImop	= 0x0008,		// fp[imop.i]
			optionFpImm		= 0x0010,		// fp[imm]
			optionFpSop1	= 0x0020,		// fp[sop1]
			optionVirtual	= 0x0040,		// imop.cls[imm]
		} ;
		struct	MnemonicInfo
		{
			InstructionCode	code ;
			const wchar_t *	mnemonic ;	// ニーモニック
			const wchar_t *	operands ;	// オペランド表記
			long			flags ;		// enum MnemonicFlag の組み合わせ
		} ;
		static const MnemonicInfo	s_MnemonicInfos[codeInstructionCount] ;

	} ;


}

#endif

