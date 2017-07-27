#include "cbase.h"
//#include "multiplayer/basenetworkedplayer_cl.h"

class C_SkeletonPlayer : public C_BasePlayer //C_BaseNetworkedPlayer
{
public:
	DECLARE_CLASS( C_SkeletonPlayer, C_BasePlayer );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void OnDataChanged( DataUpdateType_t type );

	virtual bool ShouldRegenerateOriginFromCellBits() const
	{
		// C_BasePlayer assumes that we are networking a high-res origin value instead of using a cell
		// (and so returns false here), but this is not by default the case.
		return true; // TODO: send high-precision origin instead?
	}

	const QAngle &GetRenderAngles();

	void UpdateClientSideAnimation();

private:

	QAngle m_angRender;
};