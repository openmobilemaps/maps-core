package djinni

import djinni.ast._
import djinni.generatorTools._
import djinni.meta._

class KotlinMarshal(spec: Spec) extends Marshal(spec) {

  override def typename(tm: MExpr): String = toKotlinType(tm, None)
  def typename(name: String, ty: TypeDef): String = idJava.ty(name)

  override def fqTypename(tm: MExpr): String = toKotlinType(tm, spec.javaPackage)
  def fqTypename(name: String, ty: TypeDef): String = withPackage(spec.javaPackage, idJava.ty(name))

  override def paramType(tm: MExpr): String = toKotlinValueType(tm, None)
  override def fqParamType(tm: MExpr): String = toKotlinValueType(tm, spec.javaPackage)

  override def returnType(ret: Option[TypeRef]): String = ret.fold("Unit")(ty => toKotlinValueType(ty.resolved, None))
  override def fqReturnType(ret: Option[TypeRef]): String = ret.fold("Unit")(ty => toKotlinValueType(ty.resolved, spec.javaPackage))

  override def fieldType(tm: MExpr): String = toKotlinValueType(tm, None)
  override def fqFieldType(tm: MExpr): String = toKotlinValueType(tm, spec.javaPackage)

  override def toCpp(tm: MExpr, expr: String): String = throw new AssertionError("direct java to cpp conversion not possible")
  override def fromCpp(tm: MExpr, expr: String): String = throw new AssertionError("direct cpp to java conversion not possible")

  def references(m: Meta): Seq[SymbolReference] = m match {
    case o: MOpaque =>
      o match {
        case MDate => List(ImportRef("java.util.Date"))
        case _ => List()
      }
    case e if isEnumFlags(e) => List(ImportRef("java.util.EnumSet"))
    case _ => List()
  }

  def isEnumFlags(m: Meta): Boolean = m match {
    case MDef(_, _, _, Enum(_, true)) => true
    case MExtern(_, _, _, Enum(_, true), _, _, _, _, _) => true
    case _ => false
  }
  def isEnumFlags(tm: MExpr): Boolean = tm.base match {
    case MOptional => isEnumFlags(tm.args.head)
    case _ => isEnumFlags(tm.base)
  }

  private def toKotlinValueType(tm: MExpr, packageName: Option[String]): String = {
    val name = toKotlinType(tm, packageName)
    if(isEnumFlags(tm)) s"EnumSet<$name>" else name
  }

  private def toKotlinType(tm: MExpr, packageName: Option[String]): String = {
    def args(tm: MExpr) = if (tm.args.isEmpty) "" else tm.args.map(f(_, true)).mkString("<", ", ", ">")
    def f(tm: MExpr, needRef: Boolean): String = {
      tm.base match {
        case MOptional =>
          assert(tm.args.size == 1)
          val arg = tm.args.head
          arg.base match {
            case p: MPrimitive => p.kName + "?"
            case MOptional => throw new AssertionError("nested optional?")
            case m => f(arg, true) + "?"
          }
        case e: MExtern => (if(needRef) e.java.boxed else e.java.typename) + (if(e.java.generic) args(tm) else "")
        case o =>
          val base = o match {
            case p: MPrimitive => p.kName
            case MString => "String"
            case MDate => "Date"
            case MBinary => "Array<Byte>"
            case MOptional => throw new AssertionError("optional should have been special cased")
            case MList => "ArrayList"
            case MSet => "HashSet"
            case MMap => "HashMap"
            case d: MDef => withPackage(packageName, idJava.ty(d.name))
            case e: MExtern => throw new AssertionError("unreachable")
            case p: MParam => idJava.typeParam(p.name)
          }
          base + args(tm)
      }
    }
    f(tm, false)
  }

  private def withPackage(packageName: Option[String], t: String) = packageName.fold(t)(_ + "." + t)

 }
