--[[
	Cluster extension module
]]--

function OnClusterPing(type, app, channel, session_id, msg_json)
	print(type, app,channel,session_id,msg_json)
end

function OnClusterRegister(type, app, channel, session_id, msg_json)
	print(type, app,channel,session_id,msg_json)
end

function OnClusterUnRegister(type, app, channel, session_id, msg_json)
	print(type, app,channel,session_id,msg_json)
end

function OnClusterLeave(type, app, channel, session_id, msg_json)
	print(type, app,channel,session_id,msg_json)
end

function OnClusterJoin(type, app, channel, session_id, msg_json)
	print(type, app,channel,session_id,msg_json)
end

function OnClusterPush(type, app, channel, session_id, msg_json)
	print(type, app,channel,session_id,msg_json)
end