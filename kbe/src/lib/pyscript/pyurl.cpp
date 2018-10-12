/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2018 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "script.h"
#include "pyurl.h"
#include "scriptstdouterr.h"
#include "py_macros.h"
#include "helper/profile.h"

namespace KBEngine{ namespace script {

bool PyUrl::isInit = false;
std::map<PyObject*, PyObjectPtr> PyUrl::pyCallbacks;

//-------------------------------------------------------------------------------------
bool PyUrl::initialize(Script* pScript)
{
	if(isInit)
		return true;
	
	isInit = true;

	// ע�����uuid������py
	APPEND_SCRIPT_MODULE_METHOD(pScript->getModule(),	urlopen,	__py_urlopen,	METH_VARARGS,	0);
	return isInit;
}

//-------------------------------------------------------------------------------------
void PyUrl::finalise(void)
{
	if (!isInit)
		return;

	isInit = false;
	pyCallbacks.clear();
}

//-------------------------------------------------------------------------------------
void PyUrl::onHttpCallback(bool success, const Network::Http::Request& pRequest, const std::string& data)
{
	if (!isInit)
		return;

	KBE_ASSERT(pRequest.getUserargs());

	// httpcode, data, headers, opt_success, url 
	PyObject* pyargs = PyTuple_New(5);

	PyTuple_SET_ITEM(pyargs, 0, PyLong_FromLong(pRequest.getHttpCode()));
	PyTuple_SET_ITEM(pyargs, 1, PyUnicode_FromString(pRequest.getReceivedContent()));
	PyTuple_SET_ITEM(pyargs, 2, PyUnicode_FromString(pRequest.getReceivedHeader()));
	PyTuple_SET_ITEM(pyargs, 3, PyBool_FromLong(success));
	PyTuple_SET_ITEM(pyargs, 4, PyUnicode_FromString(pRequest.url()));

	PyObject* pyRet = PyObject_CallObject((PyObject*)pRequest.getUserargs(), pyargs);
	if (pyRet == NULL)
	{
		SCRIPT_ERROR_CHECK();
	}
	else
	{
		Py_DECREF(pyRet);
	}

	pyCallbacks.erase((PyObject*)pRequest.getUserargs());
}

//-------------------------------------------------------------------------------------
PyObject* PyUrl::__py_urlopen(PyObject* self, PyObject* args)
{
	int argCount = (int)PyTuple_Size(args);
	PyObject* pyCallback = NULL;
	char* surl = NULL;
	int ret = -1;
	char* postData = NULL;
	Py_ssize_t postDataLength = 0;
	std::map<std::string, std::string> map_headers;

	if (argCount == 2)
	{
		ret = PyArg_ParseTuple(args, "s|O", &surl, &pyCallback);
	}
	else if (argCount == 3)
	{
		PyObject* pyobj = NULL;
		ret = PyArg_ParseTuple(args, "s|O|O", &surl, &pyCallback, pyobj);

		// �����headers����post data
		if (PyDict_Check(pyobj))
		{
			PyObject *key, *value;
			Py_ssize_t pos = 0;

			while (PyDict_Next(pyobj, &pos, &key, &value))
			{
				if (!PyUnicode_Check(key) || !PyUnicode_Check(value))
				{
					PyErr_Format(PyExc_AssertionError, "KBEngine::urlopen: headers should be dict(keystr : valstr)!");
					PyErr_PrintEx(0);
					return NULL;
				}

				wchar_t* PyUnicode_AsWideCharStringRet = PyUnicode_AsWideCharString(key, NULL);
				char* ckey = strutil::wchar2char(PyUnicode_AsWideCharStringRet);
				PyMem_Free(PyUnicode_AsWideCharStringRet);

				PyUnicode_AsWideCharStringRet = PyUnicode_AsWideCharString(value, NULL);
				char* cval = strutil::wchar2char(PyUnicode_AsWideCharStringRet);
				PyMem_Free(PyUnicode_AsWideCharStringRet);

				map_headers[ckey] = cval;
				free(ckey);
				free(cval);
			}
		}
		else if (PyBytes_Check(pyobj))
		{
			if (PyBytes_AsStringAndSize(pyobj, &postData, &postDataLength) < 0)
			{
				SCRIPT_ERROR_CHECK();
				return NULL;
			}
		}
		else
		{
			PyErr_Format(PyExc_AssertionError, "KBEngine::urlopen: args3 is not postData_bytes or callback!");
			PyErr_PrintEx(0);
			return NULL;
		}
	}
	else if (argCount == 4)
	{
		PyObject* pypost = NULL;
		PyObject* pyheaders = NULL;
		ret = PyArg_ParseTuple(args, "s|O|O|O", &surl, &pyCallback, pypost, pyheaders);

		// �����headers����post data
		if (PyDict_Check(pyheaders))
		{
			PyObject *key, *value;
			Py_ssize_t pos = 0;

			while (PyDict_Next(pyheaders, &pos, &key, &value))
			{
				if (!PyUnicode_Check(key) || !PyUnicode_Check(value))
				{
					PyErr_Format(PyExc_AssertionError, "KBEngine::urlopen: headers should be dict(keystr : valstr)!");
					PyErr_PrintEx(0);
					return NULL;
				}

				wchar_t* PyUnicode_AsWideCharStringRet = PyUnicode_AsWideCharString(key, NULL);
				char* ckey = strutil::wchar2char(PyUnicode_AsWideCharStringRet);
				PyMem_Free(PyUnicode_AsWideCharStringRet);

				PyUnicode_AsWideCharStringRet = PyUnicode_AsWideCharString(value, NULL);
				char* cval = strutil::wchar2char(PyUnicode_AsWideCharStringRet);
				PyMem_Free(PyUnicode_AsWideCharStringRet);

				map_headers[ckey] = cval;
				free(ckey);
				free(cval);
			}
		}
		else
		{
			PyErr_Format(PyExc_AssertionError, "KBEngine::urlopen: headers should be dict(keystr : valstr)!");
			PyErr_PrintEx(0);
			return NULL;
		}

		if (PyBytes_Check(pypost))
		{
			if (PyBytes_AsStringAndSize(pypost, &postData, &postDataLength) < 0)
			{
				SCRIPT_ERROR_CHECK();
				return NULL;
			}
		}
		else
		{
			PyErr_Format(PyExc_AssertionError, "KBEngine::urlopen: POST data should be bytes!");
			PyErr_PrintEx(0);
			return NULL;
		}
	}
	else
	{
		ret = PyArg_ParseTuple(args, "s", &surl);
	}

	if (pyCallback && !PyCallable_Check(pyCallback))
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::urlopen: invalid callback!");
		PyErr_PrintEx(0);
		return NULL;
	}

	if (surl == NULL || ret == -1)
	{
		PyErr_Format(PyExc_AssertionError, "KBEngine::urlopen: args error!");
		PyErr_PrintEx(0);
		return NULL;
	}

	Network::Http::Request* pRequest = new Network::Http::Request();

	if (pyCallback)
	{
		pyCallbacks[pyCallback] = PyObjectPtr(pyCallback);
		pRequest->setUserargs(pyCallback);
		pRequest->setCallback(onHttpCallback);
	}

	if (map_headers.size() > 0)
	{
		pRequest->setHeader(map_headers);
	}

	if (postDataLength > 0 && postData)
	{
		pRequest->setPostData(postData, postDataLength);
	}

	pRequest->setURL(surl);
	return PyLong_FromLong(Network::Http::perform(pRequest));
}

//-------------------------------------------------------------------------------------

}
}