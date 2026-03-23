- Add localisation support
- Add indication of the currently opened workspace

- Hierarchy rules
-- only one initial element is allowed (consider)
- Copy/Paste
-- paste elements at current cursor position
- hsm element selection and highlight have same effect

callback: <function>
start_timer(<timer_id>, < interval in milliseconds >, <is singleshot: true | false>)
stop_timer(<timer_id>)
restart_timer(<timer_id>)
transition(<event_id>, {arg1, arg2, …})