#pragma once
namespace ddk
{
	namespace lock {
		class CRundownProtect
		{
		private:
			EX_RUNDOWN_REF rp;
		public:
			CRundownProtect() {
				ExInitializeRundownProtection(&rp);
			};
			~CRundownProtect() {
				ExWaitForRundownProtectionRelease(&rp);
			};
		public:
			void acquire() {
				ExAcquireRundownProtection(&rp);
			}
			void release() {
				ExReleaseRundownProtection(&rp);
			}
			void wait() {
				ExWaitForRundownProtectionRelease(&rp);
			}
		};
	}
}