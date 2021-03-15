## AttributeSet
Simply replace all AttributeSet with AttributeList

## AllocaInst
###CONVERT these by adding another second argument: 0 (AddrSpace)
AllocaInst* ptr_473 = new AllocaInst(PointerTy_0, "", label_472);
AllocaInst* ptr_as_bits_726 = new AllocaInst(IntegerType::get(mod->getContext(), 32), "as_bits", label_724);

## EXPRESSION:
###\1
^
text
new AllocaInst(
text
### >> ADD HERE:
', 0'
###\2
,text
###\3
,text);
$
