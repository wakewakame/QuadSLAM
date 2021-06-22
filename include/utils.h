#pragma once
#include <utility>
#include <variant>
#include <type_traits>

namespace qs {
	template<typename T>
	struct Ok_ {
		constexpr Ok_() {}
		virtual ~Ok_() {}

		constexpr Ok_(const T& val) : val(val) {}
		constexpr Ok_(T&& val) : val(std::move(val)) {}

		constexpr Ok_(const Ok_& ok) : val(ok.val) {}
		constexpr Ok_(Ok_&& ok) : val(std::move(ok.val)) {}

		constexpr Ok_& operator=(const Ok_& ok) { val = ok.val; return *this; }
		constexpr Ok_& operator=(Ok_&& ok) { val = std::move(ok.val); return *this; }

		T val;
	};
	template<> struct Ok_<void> {};

	template<typename E>
	struct Err_ {
		constexpr Err_() {}
		virtual ~Err_() {}

		constexpr Err_(const E& val) : val(val) {}
		constexpr Err_(E&& val) : val(std::move(val)) {}

		constexpr Err_(const Err_& err) : val(err.val) {}
		constexpr Err_(Err_&& err) : val(std::move(err.val)) {}

		constexpr Err_& operator=(const Err_& err) { val = err.val; return *this; }
		constexpr Err_& operator=(Err_&& err) { val = std::move(err.val); return *this; }

		E val;
	};
	template<> struct Err_<void> {};

	template<typename T, typename CleanT = typename std::decay<T>::type>
	constexpr Ok_<CleanT> Ok(T&& val) {
		return Ok_<CleanT>(std::forward<T>(val));
	}
	constexpr Ok_<void> Ok() { return Ok_<void>(); }

	template<typename E, typename CleanE = typename std::decay<E>::type>
	constexpr Err_<CleanE> Err(E&& val) {
		return Err_<CleanE>(std::forward<E>(val));
	}
	constexpr Err_<void> Err() { return Err_<void>(); }

	template<typename T, typename E>
	struct Result {
		// SFINAEによりテンプレート引数がvoidの場合に一部のメソッドを実装しないようにする
		template<typename R> using LvalOrVoid = typename std::enable_if<!std::is_same<R, void>::value, R&>::type;
		template<typename R> using RvalOrVoid = typename std::enable_if<!std::is_same<R, void>::value, R&&>::type;

		constexpr Result() {}
		virtual ~Result() {}

		constexpr Result(const Ok_<T>& val) : val(val) {}
		constexpr Result(Ok_<T>&& val) : val(std::move(val)) {}

		constexpr Result(const Err_<E>& val) : val(val) {}
		constexpr Result(Err_<E>&& val) : val(std::move(val)) {}

		constexpr Result(const Result& result) : val(result.val) {}
		constexpr Result(Result&& result) : val(std::move(result.val)) {}

		constexpr Result& operator=(const Result& ok) { val = ok.val;            return *this; }
		constexpr Result& operator=(Result&& ok)      { val = std::move(ok.val); return *this; }

		constexpr bool is_ok()  const noexcept { return (0 == val.index()); }
		constexpr bool is_err() const noexcept { return !is_ok();           }
		constexpr explicit operator bool() const noexcept { return is_ok(); }

		template<typename U = T> constexpr LvalOrVoid<U> unwrap()                        { return std::get<Ok_<T>>(val).val;  }
		template<typename U = E> constexpr LvalOrVoid<U> unwrap_err()                    { return std::get<Err_<E>>(val).val; }
		template<typename U = T> constexpr LvalOrVoid<U> unwrap_or(const U& default_val) { return is_ok() ? unwrap() : default_val; }
		template<typename U = T> constexpr RvalOrVoid<U> unwrap_or(U&& default_val)      { return is_ok() ? unwrap() : default_val; }

		std::variant<Ok_<T>, Err_<E>> val;
	};
}
