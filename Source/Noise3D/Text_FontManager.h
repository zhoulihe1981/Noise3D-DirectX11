/***********************************************************************

					h��NoiseFontLoader

************************************************************************/

#pragma once

namespace Noise3D
{
		struct N_Font_Bitmap
		{
			UINT width;
			UINT height;
			std::vector<NVECTOR4> bitmapBuffer;
		};

		struct N_FontObject
		{
			N_FontObject();

			~N_FontObject();

			FT_Face		mFtFace;
			UINT			mFontSize;
			float				mAspectRatio;
			std::string		mInternalTextureName;//name in font mgr texture mgr
			std::vector	<NVECTOR2>	mAsciiCharSizeList;//elements will be added in GetBitmapOfChar

		};


		class /*_declspec(dllexport)*/ IFontManager
			:public IFactory<N_FontObject>,
			public IFactory<IDynamicText>,
			public IFactory<IStaticText>
		{
		public:
			friend	class IScene;//create internal object
			friend  class IRenderer;
			friend  IDynamicText;

			BOOL		CreateFontFromFile(NFilePath filePath,N_UID fontName, UINT fontSize, float fontAspectRatio = 0.707f);

			BOOL		SetFontSize(N_UID fontName, UINT  fontSize);

			BOOL		IsFontExisted(N_UID fontName);

			IStaticText*			CreateStaticTextA(N_UID fontName,N_UID textObjectName,std::string contentString, UINT boundaryWidth, UINT boundaryHeight, NVECTOR4 textColor, int wordSpacingOffset, int lineSpacingOffset);

			IStaticText*			CreateStaticTextW(N_UID fontName,  N_UID textObjectName,std::wstring contentString, UINT boundaryWidth, UINT boundaryHeight, NVECTOR4 textColor, int wordSpacingOffset, int lineSpacingOffset);

			IDynamicText*		CreateDynamicTextA(N_UID fontName, N_UID textObjectName, std::string contentString, UINT boundaryWidth, UINT boundaryHeight, NVECTOR4 textColor, int wordSpacingOffset, int lineSpacingOffset);

			NVECTOR2		GetFontSize(N_UID fontName);

			BOOL		DeleteFont(N_UID fontName);

			void			DeleteAllFont();

			BOOL		DeleteStaticText(N_UID textName);

			BOOL		DeleteStaticText(IStaticText* pText);

			BOOL		DeleteDynamicText(N_UID textName);

			BOOL		DeleteDynamicText(IDynamicText* pText);

			void			DeleteAllTexts();

			void			DeleteAllFonts();

		private:
			//init freetype library and internal objects , invoked by IScene
			BOOL	NOISE_MACRO_FUNCTION_EXTERN_CALL mFunction_Init(ITextureManager* in_created_pTexMgr, IGraphicObjectManager* in_created_pGObjMgr);
			//get bitmap of a single WCHAR
			void			mFunction_GetBitmapOfChar(N_FontObject& fontObj, wchar_t targetWChar, N_Font_Bitmap& outFontBitmap, NVECTOR4 textColor);
			//used for creating Bitmap Table (combining char pixel blocks)
			void			mFunction_GetBitmapOfString(N_FontObject& fontObj, std::wstring targetString, UINT boundaryWidth, UINT boundaryHeight, NVECTOR4 textColor, N_Font_Bitmap & outFontBitmap, int wordSpacingOffset, int lineSpacingOffset);
			//Use GetBitmapOfString() to create ascii bitmap table
			BOOL		mFunction_CreateTexture_AsciiBitmapTable(N_FontObject& fontObj,std::string fontName, UINT charWidth, UINT charHeight);

		private:

			friend IFactory<IFontManager>;

			IFontManager();

			~IFontManager();

			ITextureManager*				m_pTexMgr;//created by IScene, internal texture manager (ascii bitmap table/static Bitmap)
			IGraphicObjectManager*	m_pGraphicObjMgr;//Created by IScene, assign GObj to every TextObj

			FT_Library					m_FTLibrary;
			BOOL							mIsFTInitialized;

		};
}