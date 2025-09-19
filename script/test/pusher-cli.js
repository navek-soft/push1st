    const echo = (window.echo = new Echo({
        "broadcaster": "pusher",
        "key": "app-key",
        //      "wsHost": "192.168.1.168",
        //      "wsHost": "labns.navekscreen.video",
        "wsHost": "127.0.0.1",
        //"wsHost": "labns.navekscreen.video",

        "wsPort": 6001,
        "wsPath": "/ws",
        "wssPort": 6001,
        "forceTLS": false,
        "enabledTransports": ["ws"],
        //"enabledTransports": ["ws"],
        "auth": {
            "headers": {
                //        "Authorization" : "cvcaGDRz6YUcTWVigktV3UIQB4PNGWbhsuwiAFhAsNZm+S0fYzUs5pS57zHY4lvxZhKL7WDDN82Zrt9pLAHdR33eJLM5OJivKaaSmDq6GhnKAUtcZTHrBukA042BWBhhMt/RsVHH3d9OkHXaOXB58zW1zu1wqMoECMqAZ56M3CIaaUcKQ5jWxS2LfVkdsc2yq7Hyg613TojtXKCBdnI4a84mTO334ppqmEGygsw1/BkYqTcESs/+Hbnd02S+YxgiQVOPg8o4hxFuCuAnTn7vcSG73VLJw0x4OgbdPgKNK1wb8eC2mDcHeCGxIYWaOU4bivzgylAtoKbT7av9t89D/3j5gt4fobw7/cTsHzr6qS8VApaTb3ubat2xBW7wKntbZIIjBL4MsQWYnPQgvoK7Qi3fG86sx84ANkBGkZwv5dCflmY4dWpsdrN4ltB+6wZOnJ1FSSfhHGEsR9rRHviBbK8/pQSxO1W75/VZJNJrMGsdrbVb+/dcEEGRffcZNgNVJTqXUeHR7CDnF1hTEvPoDmHD/vkgWO7o0+qCL3ffl0dmKQGIj78Sr7AoXPhbz7eTKOAGpsin0hFN0KxtHhGmjqr/YHfvXfL2I3lfSobQCXh8FXGrWvu27Nhta/nwSt1UD81jPZZBcQ2x2G/pHbjwhO4P2nIbgHR+CWeJ8hqFD9s="
                //192.168.1.168
                //            "Authorization": "Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJhdWQiOiIxIiwianRpIjoiM2ViM2YzMDE3ODM3NzVmNGExOTgzMjU0YmVhOTUzMGFiNGM0NmI4OTBlYjMwYzc2MjUwZGZlZWU4NGNkODFmYzg3MDYwMjBkODU0ODU5OGMiLCJpYXQiOjE2MjEyNTkzOTkuNzAwMzc4LCJuYmYiOjE2MjEyNTkzOTkuNzAwMzg2LCJleHAiOjE2NTI3OTUzOTkuNDcyODAzLCJzdWIiOiIxIiwic2NvcGVzIjpbXX0.KY2VOi37k54Q43ys7Ir3PsFT91N5gRzSpIsaKN93toH1ydJJfEE0Qz5sS9UeZdV90snADDlRDDCvEbeAja0Gnyf9zic5wmZxU8obbiMnKMeA4HWpatWBkHvxzmyFHghnjtlqodi3gmCdk41NNO858Orc8pXh1cIqh0Zj0x_nnNLrZdLUg3dvM6E9CCm9uuU6nnw3yY5jsLHkDhWJYEesbElVQClp6CssVy4f9R3TkoovtPiQyatELT6lOmK8JNcfPkDVK-pPcAqI0U2MPebhZqo192kqOb93_P7HSU_eZQWXEfRYscNbsvOfaBjp0isErSoNyP8azKxeyP9HcSMoOXV2iiU4Ny3NlJROMsDuOkMKEdgY6PRI4-FuxHH-Z5CqxCzj8Q4Dh21L7IT1kOvnbCO--Q06-9cNTZB_BRzNaog3JKsgunJjMu6uvUefAD_3KoEhj3F1YKeiNCHJvbEKEWj7qmQ30qJHkCkPyuu9NZ22eWW0HvT2F5LnAICneZu9ACKermpvSqCp7x2N-OKw-Rz39sTSaosKtqxbHb5LRv_kMkhexe_vOygCbeUctJt0C1Mjj7AYjccOGePjPgsF_oQIq08QxzgSmBaOrKNvJXTf4kdzYYKeeJbaQkYYb9PBV9fPd83pM1LcT5pVpUu3D60Qih8wyTBqE8HxWIh_m9A",
                //labns.navekscreen.video
                "Authorization": "Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiIsImp0aSI6IjMxYWY5YzNmNzM5ZGFmYzMxNGM3YjIyODI1YmY0YTAxM2ZhMzZjNWNlOGNlOWI2NmViOWM5NmI1M2QwYzc4YTY0YThlYTQ2OTViNDk2NzJjIn0.eyJhdWQiOiIxIiwianRpIjoiMzFhZjljM2Y3MzlkYWZjMzE0YzdiMjI4MjViZjRhMDEzZmEzNmM1Y2U4Y2U5YjY2ZWI5Yzk2YjUzZDBjNzhhNjRhOGVhNDY5NWI0OTY3MmMiLCJpYXQiOjE2MTU1NDIyODAsIm5iZiI6MTYxNTU0MjI4MCwiZXhwIjoxNjQ3MDc4MjgwLCJzdWIiOiIyIiwic2NvcGVzIjpbXX0.FLVnepyliX48tAGs24hhzaOxjxv5BsiXkmJLB8cdLw2AOFAyZ5IXD3FgySZFHN5F-8cVZ-xBzsIuwzxHqlWnWCAUvoAomkQ5yOsM3nkULKG-F-NrE__AF0sVjwsM3GajITONQhokCI6SVTNTxvyYTQv_c7Fnmf7HpbBJyn1Z_iD_3CFMqn0-BQOkbOuXNCy2mY3E_iRF5eMn4J8Rvrq1vtvDetvky9K1_ddACIf8oYfDDIOpIi0wTwK47SH95szbvoxXVJyR7R1fRo8BtN82dXSMw7ZpAEbIwrqVEiebXHky84bWHKAKDFSSozzrjFCPmIOYO-IJP2cEGtvfUnhWF7BI9tvUlnPCnlKuhzBKc0WSPpUOXbqWnAGRZ_l6abNLOmOilkv_IsTuJtDIEPbmoggP5Oe1BWYvacNmcWLRszgNGNAwvwETMOI0FPurQudGFlYPDlMDshA-lLVdYdSYPoQvZvOlFceYRq_CPwGyGKer9bvAGk8W6a84M8HSzAEICAHBsi8YUoixlyOR_wyl5tSjg1pSF7P5yRm4-YUVTDR4TuxwjWdur8bWJdcwFkuIWyRLOICx5ipU9X80an1dMeFcQhbJMWdgrX1ffAH4fi2sGXEGOcd3wjmsoMC5Cxf06E4ETxbnPKhkNxRxG1q8EqBvU4rOHm8vLN-9NFwRRJ0"
            }
        },
        //    "authEndpoint":"http://127.0.0.1:7008/api/bridge/broadcasting/auth"
        //      "authEndpoint":"http://192.168.1.168:81/api/v3/broadcasting/auth"
        // "authEndpoint": "https://labns.navekscreen.video/api/v3/broadcasting/auth"
        "authEndpoint": "https://testmyvideo.beltelecom.by/api/v3/broadcasting/auth"
    }));
/*
    const chChat = echo.join('chat')
        .here((users) => {
            console.log("here", users);
        })
        .joining((user) => {
            console.log(user.uuid);
        })
        .leaving((user) => {
            console.log(user.uuid);
        })
        .error((error) => {
            console.error(error);
        }).listen('event_name', (e) => {
            console.log("event_name", e);
            const el = document.createElement('p');
            el.innerHTML += `<i>${JSON.stringify(e)}</i>`;
            document.querySelector('#content').appendChild(el);
        }).listenForWhisper('typing', (e) => {
            console.log(e);
        });
        */
//const channel1 = echo.private('user.2');
const channel1 = echo.listen('user.1', (data) => {
    alert(data);
});
/*
      window.channel1 = channel1;
    channel1.pusher.channels.channels['private-user.2'].bind_global(function (event, data) {
channel1.pusher.channels.channels['user.2'].bind_global(function (event, data) {
      const container = document.querySelector('#content');
        const el = document.createElement('p');
        el.innerHTML = `<b>${event}</b>&nbsp;`;
        if (data !== undefined && data.img !== undefined) {
            el.innerHTML += `<img style='display:block;' src='data:image/jpeg;base64,${data.img}'/>`;
        }
        else {
            el.innerHTML += `<i>${JSON.stringify(data)}</i>`;
        }
      container.appendChild(el);
  })
  */

function onSend() {
    alert(window.echo.broadcaster);
    chChat.whisper('typing', {
        name: "hello"
    });
}