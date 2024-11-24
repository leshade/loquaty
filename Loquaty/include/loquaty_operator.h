
#ifndef	__LOQUATY_OPERATOR_H__
#define	__LOQUATY_OPERATOR_H__	1

namespace	Loquaty
{
	namespace	Symbol
	{
		// 演算子
		enum	OperatorIndex
		{
			opInvalid	= -1,
			opAdd, opSub, opMul, opDiv, opMod,
			opBitAnd, opBitOr, opBitXor, opBitNot,
			opShiftRight, opShiftLeft,
			opEqual, opNotEqual,
			opLessEqual, opLessThan,
			opGraterEqual, opGraterThan,
			opEqualPtr, opNotEqualPtr,
			opLogicalAnd, opLogicalOr, opLogicalNot,
			opIncrement, opDecrement,
			opStore, opStoreMove,
			opStoreAdd, opStoreSub, opStoreMul, opStoreDiv, opStoreMod,
			opStoreBitAnd, opStoreBitOr, opStoreBitXor,
			opStoreShiftRight, opStoreShiftLeft,
			opStaticMemberOf, opMemberOf, opMemberCallOf,
			opParenthesis, opBracket,
			opInstanceOf, opSizeOf, opNew, opFunction,
			opConditional, opSeparator, opSequencing,
			opEndOfStatement,
			opOperatorCount,
			opComparatorFirst	= opEqual,
			opComparatorLast	= opNotEqualPtr,
			opStoreFirst		= opStore,
			opStoreLast			= opStoreShiftLeft,
		} ;

		inline bool IsComparator( OperatorIndex opIndex )
		{
			return	(opIndex >= opComparatorFirst) && (opIndex <= opComparatorLast) ;
		}
		inline bool IsStoreOperator( OperatorIndex opIndex )
		{
			return	(opIndex >= opStoreFirst) && (opIndex <= opStoreLast) ;
		}

		// 演算子優先度
		enum	OperatorPriority
		{
			priorityNo		= 0,	// 存在しない演算子
			priorityLowest,			// 最低優先度：式を末尾まで評価
			priorityList,
			priorityStore,
			priorityConditional,
			priorityLogicalOr,
			priorityLogicalAnd,
			priorityComparator,
			priorityBitShift,
			priorityBitXor,
			priorityBitOr,
			priorityBitAnd,
			priorityAdd,
			priorityMul,
			priorityPrefix,
			priorityPostfix,
			priorityMemberCall,
			priorityMember,
			priorityMemberName,
			priorityParenthesis,
			priorityHighest,
		} ;

		struct	OperatorDesc
		{
			OperatorIndex		opIndex ;
			const wchar_t *		pwszName ;
			bool				leftValue ;			// 左辺式：参照型を要求（代入操作を伴う）
			bool				overloadable ;		// オーバーロード可能か？
			const wchar_t *		pwszPairName ;		// operator 記述用追加（閉じ括弧）
			const wchar_t *		pwszFuncName ;		// native 関数名前解決用
			OperatorPriority	priorityBinary ;	// 二項演算子での優先度
			OperatorPriority	priorityUnary ;		// 単項演算子（前置）での優先度
			OperatorPriority	priorityUnaryPost ;	// 単項演算子（後置）での優先度
			OperatorIndex		opExStore ;			// 代入操作しない場合の演算子
		} ;

		extern const OperatorDesc	s_OperatorDescs[opOperatorCount] ;

	}
}

#endif


