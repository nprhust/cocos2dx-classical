/****************************************************************************
 Copyright (c) 2011 cocos2d-x.org

 http://www.cocos2d-x.org

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "CCLuaEngine.h"
#include "cocos2d.h"
#include "cocoa/CCArray.h"
#include "CCScheduler.h"
#include "cocos-ext.h"
#include "LuaBasicConversions.h"

NS_CC_BEGIN

CCLuaEngine* CCLuaEngine::m_defaultEngine = nullptr;

CCLuaEngine* CCLuaEngine::defaultEngine(void)
{
    if (!m_defaultEngine)
    {
        m_defaultEngine = new CCLuaEngine();
        m_defaultEngine->init();
    }
    return m_defaultEngine;
}

CCLuaEngine::~CCLuaEngine(void)
{
    CC_SAFE_RELEASE(m_stack);
    m_defaultEngine = nullptr;
}

bool CCLuaEngine::init(void)
{
    m_stack = CCLuaStack::create();
    CC_SAFE_RETAIN(m_stack);
    return true;
}

void CCLuaEngine::addSearchPath(const char* path)
{
    m_stack->addSearchPath(path);
}

void CCLuaEngine::addLuaLoader(lua_CFunction func)
{
    m_stack->addLuaLoader(func);
}

void CCLuaEngine::removeScriptObjectByCCObject(CCObject* pObj)
{
    m_stack->removeScriptObjectByCCObject(pObj);
}

void CCLuaEngine::removeScriptHandler(int nHandler)
{
    m_stack->removeScriptHandler(nHandler);
}

int CCLuaEngine::executeString(const char *codes)
{
    int ret = m_stack->executeString(codes);
    m_stack->clean();
    return ret;
}

int CCLuaEngine::executeScriptFile(const char* filename)
{
    int ret = m_stack->executeScriptFile(filename);
    m_stack->clean();
    return ret;
}

int CCLuaEngine::executeGlobalFunction(const char* functionName)
{
    int ret = m_stack->executeGlobalFunction(functionName);
    m_stack->clean();
    return ret;
}

int CCLuaEngine::executeMenuItemEvent(CCMenuItem* pMenuItem)
{
    int nHandler = pMenuItem->getScriptTapHandler();
    if (!nHandler) return 0;
    
    m_stack->pushInt(pMenuItem->getTag());
    m_stack->pushCCObject(pMenuItem, getLuaTypeNameByTypeId(typeid(*pMenuItem).name()));
    int ret = m_stack->executeFunctionByHandler(nHandler, 2);
    m_stack->clean();
    return ret;
}

int CCLuaEngine::executeCallFuncActionEvent(CCCallFunc* pAction, CCObject* pTarget/* = nullptr*/)
{
    ccScriptFunction& func = pAction->getScriptHandler();
    if (!func.handler) return 0;
    
    if(func.target) {
        m_stack->pushCCObject(func.target, getLuaTypeNameByTypeId(typeid(*func.target).name()));
    }
    if (pTarget) {
        m_stack->pushCCObject(pTarget, getLuaTypeNameByTypeId(typeid(*pTarget).name()));
    }
    int ret = m_stack->executeFunctionByHandler(func.handler, (pTarget ? 1 : 0) + (func.target ? 1 : 0));
    m_stack->clean();
    return ret;
}

int CCLuaEngine::executeSchedule(ccScriptFunction& func, float dt)
{
    if (!func.handler) return 0;
    
    if(func.target) {
        m_stack->pushCCObject(func.target, getLuaTypeNameByTypeId(typeid(*func.target).name()));
    }
    m_stack->pushFloat(dt);
    
    int ret = m_stack->executeFunctionByHandler(func.handler, func.target ? 2 : 1);
    m_stack->clean();
    return ret;
}

int CCLuaEngine::executeLayerTouchEvent(CCLayer* pLayer, const char* pEventName, CCTouch *pTouch)
{
    CCTouchScriptHandlerEntry* pScriptHandlerEntry = pLayer->getScriptTouchHandlerEntry();
    if (!pScriptHandlerEntry) return 0;
    ccScriptFunction& func = pScriptHandlerEntry->getHandler();
    if (!func.handler) return 0;
    
    if(func.target) {
        m_stack->pushCCObject(func.target, getLuaTypeNameByTypeId(typeid(*func.target).name()));
    }
    m_stack->pushString(pEventName);
    
    const CCPoint pt = CCDirector::sharedDirector()->convertToGL(pTouch->getLocationInView());
    m_stack->pushFloat(pt.x);
    m_stack->pushFloat(pt.y);
    int ret = m_stack->executeFunctionByHandler(func.handler, func.target ? 4 : 3);
    m_stack->clean();
    return ret;
}

int CCLuaEngine::executeLayerTouchesEvent(CCLayer* pLayer, const char* pEventName, CCSet *pTouches)
{
    CCTouchScriptHandlerEntry* pScriptHandlerEntry = pLayer->getScriptTouchHandlerEntry();
    if (!pScriptHandlerEntry) return 0;
    ccScriptFunction& func = pScriptHandlerEntry->getHandler();
    if (!func.handler) return 0;
    
    if(func.target) {
        m_stack->pushCCObject(func.target, getLuaTypeNameByTypeId(typeid(*func.target).name()));
    }
    m_stack->pushString(pEventName);
    
    CCDirector* pDirector = CCDirector::sharedDirector();
    lua_State *L = m_stack->getLuaState();
    lua_newtable(L);
    int i = 1;
    for (CCSetIterator it = pTouches->begin(); it != pTouches->end(); ++it)
    {
        CCTouch* pTouch = (CCTouch*)*it;
        CCPoint pt = pDirector->convertToGL(pTouch->getLocationInView());
        lua_pushnumber(L, pt.x);
        lua_rawseti(L, -2, i++);
        lua_pushnumber(L, pt.y);
        lua_rawseti(L, -2, i++);
        lua_pushinteger(L, pTouch->getID());
        lua_rawseti(L, -2, i++);
    }
    int ret = m_stack->executeFunctionByHandler(func.handler, func.target ? 3 : 2);
    m_stack->clean();
    return ret;
}

int CCLuaEngine::executeAccelerometerEvent(ccScriptFunction& func, CCAcceleration* pAccelerationValue)
{
    if (!func.handler) return 0;
    
    if(func.target) {
        m_stack->pushCCObject(func.target, getLuaTypeNameByTypeId(typeid(*func.target).name()));
    }
    
    m_stack->pushFloat(pAccelerationValue->x);
    m_stack->pushFloat(pAccelerationValue->y);
    m_stack->pushFloat(pAccelerationValue->z);
    m_stack->pushFloat(pAccelerationValue->timestamp);
    int ret = m_stack->executeFunctionByHandler(func.handler, func.target ? 5 : 4);
    m_stack->clean();
    return ret;
}

int CCLuaEngine::executeEvent(ccScriptFunction& func, const char* pEventName) {
    if(func.target) {
        m_stack->pushCCObject(func.target, getLuaTypeNameByTypeId(typeid(*func.target).name()));
    }
    m_stack->pushString(pEventName);
    int ret = m_stack->executeFunctionByHandler(func.handler, func.target ? 2 : 1);
    m_stack->clean();
    return ret;
}

bool CCLuaEngine::handleAssert(const char *msg)
{
    bool ret = m_stack->handleAssert(msg);
    m_stack->clean();
    return ret;
}

int CCLuaEngine::reallocateScriptHandler(int nHandler)
{    
    int nRet = m_stack->reallocateScriptHandler(nHandler);
    m_stack->clean();
    return nRet;
}

int CCLuaEngine::executeTableViewEvent(int nEventType,cocos2d::extension::CCTableView* pTableView,void* pValue, CCArray* pResultArray)
{
    if (nullptr == pTableView)
        return 0;
    
    int nHanlder = pTableView->getScriptHandler(nEventType);
    if (0 == nHanlder)
        return 0;
    
    int nRet = 0;
    switch (nEventType)
    {
        case cocos2d::extension::CCTableView::kTableViewScroll:
        case cocos2d::extension::CCTableView::kTableViewZoom:
            {
                m_stack->pushCCObject(pTableView, getLuaTypeNameByTypeId(typeid(*pTableView).name()));
                nRet = m_stack->executeFunctionByHandler(nHanlder, 1);
            }
            break;
        case cocos2d::extension::CCTableView::kTableCellTouched:
        case cocos2d::extension::CCTableView::kTableCellHighLight:
        case cocos2d::extension::CCTableView::kTableCellUnhighLight:
        case cocos2d::extension::CCTableView::kTableCellWillRecycle:
            {
                m_stack->pushCCObject(pTableView, getLuaTypeNameByTypeId(typeid(*pTableView).name()));
                m_stack->pushCCObject(static_cast<cocos2d::extension::CCTableViewCell*>(pValue), getLuaTypeNameByTypeId(typeid(*pValue).name()));
                nRet = m_stack->executeFunctionByHandler(nHanlder, 2);
            }
            break;
        case cocos2d::extension::CCTableView::kTableCellSizeForIndex:
            {
                m_stack->pushCCObject(pTableView, getLuaTypeNameByTypeId(typeid(*pTableView).name()));
                m_stack->pushInt(*((int*)pValue));
                nRet = m_stack->executeFunctionReturnArray(nHanlder, 2, 2, pResultArray);
            }
            break;
        case cocos2d::extension::CCTableView::kTableCellSizeAtIndex:
            {
                m_stack->pushCCObject(pTableView, getLuaTypeNameByTypeId(typeid(*pTableView).name()));
                m_stack->pushInt(*((int*)pValue));
                nRet = m_stack->executeFunctionReturnArray(nHanlder, 2, 1, pResultArray);
            }
            break;
        case cocos2d::extension::CCTableView::kNumberOfCellsInTableView:
            {
                m_stack->pushCCObject(pTableView, getLuaTypeNameByTypeId(typeid(*pTableView).name()));
                nRet = m_stack->executeFunctionReturnArray(nHanlder, 1, 1, pResultArray);
            }
            break;
        default:
            break;
    }
    return nRet;
}

int CCLuaEngine::executeEventWithArgs(ccScriptFunction& func, CCArray* pArgs) {
    int nArgNums = 0;
    
    // target
    if(func.target) {
        m_stack->pushCCObject(func.target, getLuaTypeNameByTypeId(typeid(*func.target).name()));
        nArgNums++;
    }
    
    // push args
    CCObject*   pObject = nullptr;
    CCInteger*  pIntVal = nullptr;
    CCString*   pStrVal = nullptr;
    CCDouble*   pDoubleVal = nullptr;
    CCFloat*    pFloatVal = nullptr;
    CCBool*     pBoolVal = nullptr;
    if(pArgs) {
        for (unsigned int i = 0; i < pArgs->count(); i++) {
            pObject = pArgs->objectAtIndex(i);
            if (nullptr != (pIntVal = dynamic_cast<CCInteger*>(pObject))) {
                m_stack->pushInt(pIntVal->getValue());
                nArgNums++;
            } else if (nullptr != (pStrVal = dynamic_cast<CCString*>(pObject))) {
                m_stack->pushString(pStrVal->getCString());
                nArgNums++;
            } else if (nullptr != (pDoubleVal = dynamic_cast<CCDouble*>(pObject))) {
                m_stack->pushFloat(pDoubleVal->getValue());
                nArgNums++;
            } else if (nullptr != (pFloatVal = dynamic_cast<CCFloat*>(pObject))) {
                m_stack->pushFloat(pFloatVal->getValue());
                nArgNums++;
            } else if (nullptr != (pBoolVal = dynamic_cast<CCBool*>(pObject))) {
                m_stack->pushBoolean(pBoolVal->getValue());
                nArgNums++;
            } else if(nullptr != pObject) {
                m_stack->pushCCObject(pObject, getLuaTypeNameByTypeId(typeid(*pObject).name()));
                nArgNums++;
            }
        }
    }
    
    return  m_stack->executeFunctionByHandler(func.handler, nArgNums);
}

bool CCLuaEngine::parseConfig(CCScriptEngineProtocol::ConfigType type, const std::string& str)
{
    lua_getglobal(m_stack->getLuaState(), "__onParseConfig");
    if (!lua_isfunction(m_stack->getLuaState(), -1))
    {
        CCLOG("[LUA ERROR] name '%s' does not represent a Lua function", "__onParseConfig");
        lua_pop(m_stack->getLuaState(), 1);
        return false;
    }
    
    m_stack->pushInt((int)type);
    m_stack->pushString(str.c_str());
    
    return m_stack->executeFunction(2);
}

NS_CC_END
