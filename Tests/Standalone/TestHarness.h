// Copyright (c) 2026 Moe. MIT licensed. See LICENSE.
//
// A deliberately tiny assertion harness. The rules layer has no engine
// dependency, so its tests should have no framework dependency either: this
// builds and runs anywhere a C++17 compiler does.

#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace MaskGameTest
{
	struct FContext
	{
		int Checks = 0;
		int Failures = 0;
		std::string CurrentCase;
	};

	inline FContext& Ctx()
	{
		static FContext Instance;
		return Instance;
	}

	inline void BeginCase(const std::string& Name)
	{
		Ctx().CurrentCase = Name;
		std::printf("  - %s\n", Name.c_str());
	}

	inline void Fail(const char* File, int Line, const std::string& Message)
	{
		++Ctx().Failures;
		std::printf("    FAIL %s:%d  %s\n", File, Line, Message.c_str());
	}

	inline void CheckTrue(bool Condition, const char* Expr, const char* File, int Line)
	{
		++Ctx().Checks;
		if (!Condition)
		{
			Fail(File, Line, std::string("expected true: ") + Expr);
		}
	}

	template <typename A, typename B>
	void CheckEqual(const A& Actual, const B& Expected, const char* Expr, const char* File, int Line)
	{
		++Ctx().Checks;
		if (!(Actual == Expected))
		{
			Fail(File, Line, std::string(Expr) + ": got " + std::to_string(Actual) + ", expected " + std::to_string(Expected));
		}
	}

	inline void CheckNear(double Actual, double Expected, double Tolerance, const char* Expr, const char* File, int Line)
	{
		++Ctx().Checks;
		if (std::fabs(Actual - Expected) > Tolerance)
		{
			Fail(File, Line, std::string(Expr) + ": got " + std::to_string(Actual) + ", expected " + std::to_string(Expected));
		}
	}

	inline int Report(const char* SuiteName)
	{
		const FContext& C = Ctx();
		std::printf("%s: %d checks, %d failures\n", SuiteName, C.Checks, C.Failures);
		return C.Failures == 0 ? 0 : 1;
	}
}

#define TEST_CASE(Name)          ::MaskGameTest::BeginCase(Name)
#define CHECK(Expr)              ::MaskGameTest::CheckTrue((Expr), #Expr, __FILE__, __LINE__)
#define CHECK_EQ(A, B)           ::MaskGameTest::CheckEqual((A), (B), #A " == " #B, __FILE__, __LINE__)
#define CHECK_NEAR(A, B, Tol)    ::MaskGameTest::CheckNear((A), (B), (Tol), #A " ~= " #B, __FILE__, __LINE__)
