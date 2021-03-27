#ifndef ANYMOVABLE_H
#define ANYMOVABLE_H

#include <memory>


namespace NSLibrary {

//---------------------------------------------------------------------------
// How To Use
//---------------------------------------------------------------------------
//
// Description
// CAnyMovable allows you to store any movable only class without
// any restrictions on the class. It also allows you to provide
// an interfaceto a class and implementations for different classes of
// the method.
//
// The operator->() goes without any checks and may fail if the object
// is empty, that is, does not store anything. It is your responsibility
// to call isDefined() method before accessing the interface.
//
// class CAnyMovable does not use Small Object Optimization.
// In particular, move operations are always cheap.
//
// CAnyMovable has value semantics and extends the ideas of Sean Parent's
// talk on cppcon about Run-time Polymorphism.
//
// 1) Create an interface class:
//
// template<class TBase>
// class IAny : public TBase {
// public:
//   virtual void print() const = 0;
// };
//
// The class discribes the interface of an abstract object you want to support
// Do not use names with prefix underscore here, e.g.,
// BAD: virtual void _print() const = 0;
// GOOD: virtual void print() const = 0;
//
// This interface will be accessible when using operator->() on CAnyObject.
// The example is below.
//
// 2) Create an implementation class:
//
// template<class TBase, class TObject>
// class CAnyImpl : public TBase {
//   using CBase = TBase;
// public:
//   using CBase::CBase;
//   void print() const override {
//     std::cout << "data = " << CBase::Object() << std::endl;
//   }
// };
//
// Here you implement all the functions from the interface.
//
// The parameter TObject is used to reimplement the behaviour for different
// types of objects if needed. In the example above, it may happen that
// TObject does not support stream operator << and you want to define
// a specialization of the implementation class.
//
// 3) Create your Any class:
//
// class CAny : public CAnyMovable<IAny, CAnyImpl> {
//   using CBase = CAnyMovable<IAny, CAnyImpl>;
// public:
//   using CBase::CBase;
//   friend bool operator==(const CAny&, const CAny&) {
//     ...
//   }
// };
//
// You can add any additional functionality to your CAny class.
//
// Now you can use it like this:
// CAny x = 'c';
// x->print();
// x = std::move("123");
// x->print();
// x = 1.45;
// x->print();
//
//
// If you want to store an object of Type R and construct it on the fly
// from the data: x, y, z
// Then use emplace function like this
// CAny s;
// s.emplace<R>(x, y, z);
// Then the object of type R will be created without creation of
// intermediate objects
//
//
//---------------------------------------------------------------------------


template< template<class>class TInterface,
          template<class, class>class TImplementation>
class CAnyMovable {
  class IObjectStored;
  using CStoredPtr = std::unique_ptr<IObjectStored>;
  template<class T>
  using CObjType = std::remove_reference_t<T>;
public:
  CAnyMovable() = default;

  template<class T>
  CAnyMovable(T&& Object)
    : pIObject_(std::make_unique<CObjectStored<CObjType<T>>>(std::forward<T>(Object))) {
  }

  template<class T, class... TArgs>
  CAnyMovable(std::in_place_type_t<T>,TArgs&& ... args)
    : pIObject_(std::make_unique<CObjectStored<T>>(std::forward<TArgs>(args)...)) {}


  CAnyMovable(const CAnyMovable& Other) = delete;

  CAnyMovable(CAnyMovable&& Other) noexcept = default;

  CAnyMovable& operator=(const CAnyMovable& Other) = delete;

  CAnyMovable& operator=(CAnyMovable&& Other) noexcept = default;

  bool isDefined() const {
    return pIObject_.operator bool();
  }

  // I need this to avoid explicit declaration of a virtual distructor
  // on the user side
  class IEmpty {
  protected:
    virtual ~IEmpty() = default;
  };

  TInterface<IEmpty>* operator->() {
    return pIObject_.get();
  }

  TInterface<IEmpty>* operator->() const {
    return pIObject_.get();
  }

  template<class TObject, class... TArgs>
  void emplace(TArgs&& ... args) {
    pIObject_ = std::make_unique<CObjectStored<TObject>>(std::forward<TArgs>(args)...);
  }

  void clear() {
    pIObject_.reset();
  }

protected:
  ~CAnyMovable() = default;

private:
  class IObjectStored : public TInterface<IEmpty> {
  public:
    inline ~IObjectStored() override = default;
  protected:
    friend class CAnyMoveable;
  };


  template<class TObject>
  class CObjectKeeper : public IObjectStored {
  public:
    using CObjectType = TObject;

    CObjectKeeper(TObject&& Object) noexcept
      : Object_(std::move(Object)) {
    }

    ~CObjectKeeper() override = default;

    template<class... TArgs>
    CObjectKeeper(TArgs&& ... args) : Object_(std::forward<TArgs>(args)...) {}

  protected:
    TObject& Object() {
      return Object_;
    }

    const TObject& Object() const {
      return Object_;
    }

  private:
    TObject Object_;
  };

  // Specialization for arrays
  template<class T, size_t Tsize>
  class CObjectKeeper<T[Tsize]> : public IObjectStored {
  public:
    using CBaseType = T;
    using CObjectType = CBaseType[Tsize];

    CObjectKeeper(CObjectType&& Object) noexcept
      : CObjectKeeper(std::move(Object), std::make_integer_sequence <size_t, Tsize> {}) {
    }

    ~CObjectKeeper() override = default;

  protected:
    CObjectType& Object() {
      return Object_;
    }

    const CObjectType& Object() const {
      return Object_;
    }
  private:

    template<size_t... TIndex>
    CObjectKeeper(CObjectType&& Object, std::integer_sequence<size_t, TIndex...>) noexcept
      : Object_ {
      std::move(Object[TIndex])...
    } {}

    CObjectType Object_;
  };


  template<class TObject>
  class CObjectStored :
    public TImplementation<CObjectKeeper<TObject>, TObject> {
    using CBase = TImplementation<CObjectKeeper<TObject>, TObject>;
  public:
    using CBase::CBase;

    inline ~CObjectStored() override = default;
  };

protected:
  CStoredPtr& StoredPtr() {
    return pIObject_;
  }

  const CStoredPtr& StoredPtr() const {
    return pIObject_;
  }

private:
  CStoredPtr pIObject_;
};

} // NSLibrary

#endif // ANYMOVABLE_H
