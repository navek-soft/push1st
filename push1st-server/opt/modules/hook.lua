--[[
	Channel events hook example
]]--
function OnTrigger(type, app, channel, session_id, msg_json)
	print(type, app,channel,session_id,msg_json)
end