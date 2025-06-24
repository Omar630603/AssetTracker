import AppLogoIcon from '@/components/app-logo-icon';
import { type SharedData } from '@/types';
import { Head, Link, usePage } from '@inertiajs/react';

export default function Welcome() {
    const { auth } = usePage<SharedData>().props;

    return (
        <>
            <Head title="Welcome">
                <link rel="preconnect" href="https://fonts.bunny.net" />
                <link href="https://fonts.bunny.net/css?family=instrument-sans:400,500,600" rel="stylesheet" />
            </Head>
            <div className="flex min-h-screen flex-col items-center bg-[#FDFDFC] p-6 text-[#1b1b18] lg:justify-center lg:p-8 dark:bg-[#0a0a0a]">
                <header className="mb-6 w-full max-w-[335px] text-sm not-has-[nav]:hidden lg:max-w-4xl">
                    <nav className="flex items-center justify-end gap-4">
                        {auth.user ? (
                            <Link
                                href={route('dashboard')}
                                className="inline-block rounded-sm border border-[#19140035] px-5 py-1.5 text-sm leading-normal text-[#1b1b18] hover:border-[#1915014a] dark:border-[#3E3E3A] dark:text-[#EDEDEC] dark:hover:border-[#62605b]"
                            >
                                Dashboard
                            </Link>
                        ) : (
                            <>
                                <Link
                                    href={route('login')}
                                    className="inline-block rounded-sm border border-transparent px-5 py-1.5 text-sm leading-normal text-[#1b1b18] hover:border-[#19140035] dark:text-[#EDEDEC] dark:hover:border-[#3E3E3A]"
                                >
                                    Log in
                                </Link>
                                <Link
                                    href={route('register')}
                                    className="inline-block rounded-sm border border-[#19140035] px-5 py-1.5 text-sm leading-normal text-[#1b1b18] hover:border-[#1915014a] dark:border-[#3E3E3A] dark:text-[#EDEDEC] dark:hover:border-[#62605b]"
                                >
                                    Register
                                </Link>
                            </>
                        )}
                    </nav>
                </header>
                <div className="flex w-full items-center justify-center opacity-100 transition-opacity duration-750 lg:grow starting:opacity-0">
                    <main className="flex w-full max-w-[335px] flex-col-reverse lg:max-w-4xl lg:flex-row">
                        <div className="flex-1 flex flex-col justify-between h-full rounded-xl bg-white p-7 pb-10 text-[13px] leading-[20px] shadow-[inset_0_0_0_1px_rgba(26,26,0,0.11)] lg:rounded-tl-lg lg:rounded-br-none lg:p-16 dark:bg-[#161615] dark:text-[#EDEDEC] dark:shadow-[inset_0_0_0_1px_#3b3a7e30]">
                            {/* Top: logo/title/desc */}
                            <div className="flex flex-col items-center">
                                <span className="bg-sidebar-primary text-sidebar-primary-foreground flex aspect-square items-center justify-center rounded-md size-14 mb-4">
                                    <AppLogoIcon className="size-8 text-white dark:text-black" />
                                </span>
                                <h1 className="text-2xl font-semibold mb-2 text-center tracking-tight">
                                    Asset Tracker
                                </h1>
                                <p className="mb-4 text-[14px] text-center text-muted-foreground dark:text-[#b2b3d3]">
                                    This project helps you track and manage valuable assets using BLE <span className="text-[#6366F1] font-semibold">readers</span> (ESP32) and <span className="text-[#6366F1] font-semibold">tags</span>.
                                </p>
                            </div>
                            {/* Divider */}
                            <div className="w-full border-t border-dashed border-[#e3e3e0] dark:border-[#22223a] my-4" />

                            {/* Features */}
                            <ul className="space-y-3 text-[14.3px] text-left">
                                <li className="flex gap-2 items-start">
                                    <span className="mt-0.5 text-lg">🔖</span>
                                    <span>
                                        <span className="font-medium text-[#6366F1]">Tags</span> are attached to assets and broadcast BLE signals.
                                    </span>
                                </li>
                                <li className="flex gap-2 items-start">
                                    <span className="mt-0.5 text-lg">📡</span>
                                    <span>
                                        <span className="font-medium text-[#6366F1]">Readers</span> pick up signals, measure <span className="font-medium">RSSI</span>, apply a Kalman filter, and report to the server.
                                    </span>
                                </li>
                                <li className="flex gap-2 items-start">
                                    <span className="mt-0.5 text-lg">🗺️</span>
                                    <span>
                                        Automatic <span className="font-medium text-[#6366F1]">location logs</span> and movement tracking for every asset.
                                    </span>
                                </li>
                                <li className="flex gap-2 items-start">
                                    <span className="mt-0.5 text-lg">⚙️</span>
                                    <span>
                                        <span className="font-medium text-[#6366F1]">Log in</span> to configure readers, add assets, and view logs and statistics.
                                    </span>
                                </li>
                            </ul>
                        </div>
                    </main>
                </div>
                <div className="hidden h-14.5 lg:block"></div>
            </div>
        </>
    );
}
